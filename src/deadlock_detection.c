/* =============================================================================
 * DEADLOCK_DETECTION.C - Deadlock Detection Engine Implementation
 * =============================================================================
 * Implementation of main deadlock detection logic that integrates process
 * monitoring, graph building, and cycle detection.
 * =============================================================================
 */

#include "deadlock_detection.h"
#include "process_monitor.h"
#include "utility.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* =============================================================================
 * HELPER FUNCTIONS
 * =============================================================================
 */

/*
 * is_pid_in_array - Check if PID exists in array
 * @pids: Array of PIDs
 * @count: Number of PIDs in array
 * @pid: PID to search for
 * @return: 1 if found, 0 otherwise
 */
static int is_pid_in_array(const int* pids, int count, int pid)
{
    if (pids == NULL) {
        return 0;
    }
    
    for (int i = 0; i < count; i++) {
        if (pids[i] == pid) {
            return 1;
        }
    }
    
    return 0;
}

/*
 * add_pid_to_array - Add PID to array if not already present
 * @pids: Pointer to array of PIDs
 * @count: Pointer to current count
 * @capacity: Pointer to current capacity
 * @pid: PID to add
 * @return: SUCCESS (0) on success, negative on error
 */
static int add_pid_to_array(int** pids, int* count, int* capacity, int pid)
{
    if (pids == NULL || count == NULL || capacity == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Check if already present */
    if (is_pid_in_array(*pids, *count, pid)) {
        return SUCCESS; /* Already in array */
    }
    
    /* Expand array if needed */
    if (*count >= *capacity) {
        int new_capacity = *capacity == 0 ? 10 : *capacity * 2;
        int* new_pids = (int*)safe_realloc(*pids, sizeof(int) * new_capacity);
        if (new_pids == NULL) {
            return ERROR_OUT_OF_MEMORY;
        }
        *pids = new_pids;
        *capacity = new_capacity;
    }
    
    (*pids)[*count] = pid;
    (*count)++;
    
    return SUCCESS;
}

/* =============================================================================
 * MAIN FUNCTIONS
 * =============================================================================
 */

/*
 * create_deadlock_report - Allocate and initialize a new DeadlockReport
 * @return: Pointer to allocated DeadlockReport, or NULL on failure
 * Description: Creates a new DeadlockReport structure with all fields initialized.
 *              Time complexity: O(1)
 * Error handling: Returns NULL on allocation failure
 */
DeadlockReport* create_deadlock_report(void)
{
    DeadlockReport* report = (DeadlockReport*)safe_malloc(sizeof(DeadlockReport));
    if (report == NULL) {
        return NULL;
    }
    
    memset(report, 0, sizeof(DeadlockReport));
    report->deadlock_detected = 0;
    report->num_deadlocked = 0;
    report->num_cycles = 0;
    report->num_explanations = 0;
    report->num_recommendations = 0;
    report->timestamp = (int)time(NULL);
    report->total_processes_scanned = 0;
    report->total_resources_found = 0;
    
    return report;
}

/*
 * build_rag_from_processes - Build Resource Allocation Graph from process info
 * @procs: Array of ProcessResourceInfo structures
 * @num_procs: Number of processes
 * @graph: Output parameter for created RAG
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Constructs RAG by adding processes, resources, and edges
 *              based on ProcessResourceInfo data. Creates vertices and edges
 *              for allocations and requests.
 *              Time complexity: O(P * R) where P=processes, R=resources per process
 * Error handling: Returns error codes for allocation failures or invalid data
 */
int build_rag_from_processes(ProcessResourceInfo* procs, int num_procs,
                             ResourceGraph** graph)
{
    if (procs == NULL || graph == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (num_procs <= 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Estimate max vertices: processes + resources */
    int max_vertices = num_procs * 2; /* Rough estimate */
    if (max_vertices > MAX_VERTICES) {
        max_vertices = MAX_VERTICES;
    }
    
    /* Create graph */
    *graph = create_graph(max_vertices);
    if (*graph == NULL) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Count resources for statistics */
    int resource_count = 0;
    
    /* Add processes and resources, create edges */
    for (int i = 0; i < num_procs; i++) {
        int pid = procs[i].pid;
        
        /* Add process vertex */
        int process_vertex = add_process_vertex(*graph, pid);
        if (process_vertex < 0) {
            free_graph(*graph);
            *graph = NULL;
            return ERROR_GRAPH_CREATION_FAILED;
        }
        
        /* Add allocation edges (resources held by process) */
        for (int j = 0; j < procs[i].num_held; j++) {
            int rid = procs[i].held_resources[j];
            
            /* Add resource vertex (default to single instance) */
            int resource_vertex = add_resource_vertex(*graph, rid, 1);
            if (resource_vertex < 0) {
                free_graph(*graph);
                *graph = NULL;
                return ERROR_GRAPH_CREATION_FAILED;
            }
            
            resource_count++;
            
            /* Add allocation edge: R->P */
            int result = add_allocation_edge(*graph, rid, pid);
            if (result != SUCCESS) {
                free_graph(*graph);
                *graph = NULL;
                return result;
            }
        }
        
        /* Add request edges (resources process is waiting for) */
        for (int j = 0; j < procs[i].num_waiting; j++) {
            int rid = procs[i].waiting_resources[j];
            
            /* Add resource vertex if not already present */
            int resource_vertex = add_resource_vertex(*graph, rid, 1);
            if (resource_vertex < 0) {
                free_graph(*graph);
                *graph = NULL;
                return ERROR_GRAPH_CREATION_FAILED;
            }
            
            if (!is_pid_in_array(procs[i].held_resources, procs[i].num_held, rid)) {
                resource_count++;
            }
            
            /* Add request edge: P->R */
            int result = add_request_edge(*graph, pid, rid);
            if (result != SUCCESS) {
                free_graph(*graph);
                *graph = NULL;
                return result;
            }
        }
    }
    
    /* Update statistics */
    if (*graph != NULL) {
        int num_processes, num_resources, num_edges;
        get_graph_statistics(*graph, &num_processes, &num_resources, &num_edges);
        /* Resource count already tracked above */
    }
    
    return SUCCESS;
}

/*
 * is_deadlock_definite - Check if a cycle represents a definite deadlock
 * @cycle: CycleInfo to analyze
 * @graph: ResourceGraph for resource instance information
 * @return: 1 if definite deadlock, 0 if potential deadlock
 * Description: Determines if cycle involves only single-instance resources
 *              (definite deadlock) or includes multi-instance resources
 *              (potential deadlock that might resolve).
 *              Time complexity: O(cycle_length)
 * Error handling: Returns 0 for invalid cycles
 */
int is_deadlock_definite(const CycleInfo* cycle, const ResourceGraph* graph)
{
    if (cycle == NULL || graph == NULL) {
        return 0;
    }
    
    if (cycle->cycle_path == NULL || cycle->cycle_length == 0) {
        return 0;
    }
    
    /* Check each resource in cycle */
    for (int i = 0; i < cycle->cycle_length - 1; i++) { /* Exclude last (duplicate) */
        int vertex = cycle->cycle_path[i];
        
        if (vertex >= 0 && vertex < graph->num_vertices) {
            if (graph->vertex_type[vertex] == VERTEX_TYPE_RESOURCE) {
                /* If any resource has multiple instances, it's potential deadlock */
                if (graph->vertex_instances[vertex] > 1) {
                    return 0; /* Potential deadlock */
                }
            }
        }
    }
    
    /* All resources are single-instance = definite deadlock */
    return 1;
}

/*
 * identify_deadlocked_processes - Extract process IDs from cycles
 * @cycles: Array of CycleInfo structures
 * @num_cycles: Number of cycles
 * @graph: ResourceGraph for vertex lookup
 * @pids: Output parameter for array of deadlocked PIDs
 * @count: Output parameter for number of unique PIDs
 * @return: SUCCESS (0) on success, negative on error
 * Description: Extracts all unique process IDs involved in deadlock cycles.
 *              Allocates array for PIDs. Caller must free result.
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Returns error codes for allocation failures
 */
int identify_deadlocked_processes(const CycleInfo* cycles, int num_cycles,
                                  const ResourceGraph* graph, int** pids, int* count)
{
    if (cycles == NULL || graph == NULL || pids == NULL || count == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *pids = NULL;
    *count = 0;
    int capacity = 0;
    
    /* Extract unique PIDs from all cycles */
    for (int i = 0; i < num_cycles; i++) {
        if (cycles[i].cycle_path == NULL) {
            continue;
        }
        
        /* Extract PIDs from cycle path */
        for (int j = 0; j < cycles[i].cycle_length - 1; j++) { /* Exclude last */
            int vertex = cycles[i].cycle_path[j];
            
            if (vertex >= 0 && vertex < graph->num_vertices) {
                if (graph->vertex_type[vertex] == VERTEX_TYPE_PROCESS) {
                    int pid = graph->vertex_id[vertex];
                    int result = add_pid_to_array(pids, count, &capacity, pid);
                    if (result != SUCCESS) {
                        free(*pids);
                        *pids = NULL;
                        *count = 0;
                        return result;
                    }
                }
            }
        }
        
        /* Also use process_ids array if available */
        if (cycles[i].process_ids != NULL) {
            for (int j = 0; j < cycles[i].num_processes; j++) {
                int pid = cycles[i].process_ids[j];
                int result = add_pid_to_array(pids, count, &capacity, pid);
                if (result != SUCCESS) {
                    free(*pids);
                    *pids = NULL;
                    *count = 0;
                    return result;
                }
            }
        }
    }
    
    return SUCCESS;
}

/*
 * filter_actual_deadlocks - Distinguish definite vs potential deadlocks
 * @cycles: Array of CycleInfo structures
 * @num_cycles: Number of cycles
 * @graph: ResourceGraph for resource instance information
 * @definite_deadlocks: Output parameter for definite deadlock cycles
 * @num_definite: Output parameter for number of definite deadlocks
 * @potential_deadlocks: Output parameter for potential deadlock cycles
 * @num_potential: Output parameter for number of potential deadlocks
 * @return: SUCCESS (0) on success, negative on error
 * Description: Categorizes cycles into definite deadlocks (single-instance
 *              resources) and potential deadlocks (multi-instance resources).
 *              Time complexity: O(C * L)
 * Error handling: Returns error codes for allocation failures
 */
int filter_actual_deadlocks(const CycleInfo* cycles, int num_cycles,
                           const ResourceGraph* graph,
                           CycleInfo** definite_deadlocks, int* num_definite,
                           CycleInfo** potential_deadlocks, int* num_potential)
{
    if (cycles == NULL || graph == NULL || 
        definite_deadlocks == NULL || num_definite == NULL ||
        potential_deadlocks == NULL || num_potential == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *definite_deadlocks = NULL;
    *potential_deadlocks = NULL;
    *num_definite = 0;
    *num_potential = 0;
    
    int definite_capacity = 0;
    int potential_capacity = 0;
    
    /* Categorize each cycle */
    for (int i = 0; i < num_cycles; i++) {
        int is_definite = is_deadlock_definite(&cycles[i], graph);
        
        if (is_definite) {
            /* Add to definite deadlocks */
            if (*num_definite >= definite_capacity) {
                int new_capacity = definite_capacity == 0 ? 10 : definite_capacity * 2;
                CycleInfo* new_list = (CycleInfo*)safe_realloc(
                    *definite_deadlocks, sizeof(CycleInfo) * new_capacity);
                if (new_list == NULL) {
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                *definite_deadlocks = new_list;
                definite_capacity = new_capacity;
            }
            
            /* Copy cycle structure */
            CycleInfo* dest = &(*definite_deadlocks)[*num_definite];
            memset(dest, 0, sizeof(CycleInfo));
            
            /* Copy cycle data */
            dest->cycle_length = cycles[i].cycle_length;
            dest->cycle_start_vertex = cycles[i].cycle_start_vertex;
            dest->cycle_end_vertex = cycles[i].cycle_end_vertex;
            dest->num_processes = cycles[i].num_processes;
            dest->num_resources = cycles[i].num_resources;
            
            /* Deep copy cycle_path */
            if (cycles[i].cycle_path != NULL && cycles[i].cycle_length > 0) {
                dest->cycle_path = (int*)safe_malloc(sizeof(int) * cycles[i].cycle_length);
                if (dest->cycle_path == NULL) {
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->cycle_path, cycles[i].cycle_path,
                       sizeof(int) * cycles[i].cycle_length);
            }
            
            /* Deep copy process_ids */
            if (cycles[i].process_ids != NULL && cycles[i].num_processes > 0) {
                dest->process_ids = (int*)safe_malloc(sizeof(int) * cycles[i].num_processes);
                if (dest->process_ids == NULL) {
                    free(dest->cycle_path);
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->process_ids, cycles[i].process_ids,
                       sizeof(int) * cycles[i].num_processes);
            }
            
            /* Deep copy resource_ids */
            if (cycles[i].resource_ids != NULL && cycles[i].num_resources > 0) {
                dest->resource_ids = (int*)safe_malloc(sizeof(int) * cycles[i].num_resources);
                if (dest->resource_ids == NULL) {
                    free(dest->cycle_path);
                    free(dest->process_ids);
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->resource_ids, cycles[i].resource_ids,
                       sizeof(int) * cycles[i].num_resources);
            }
            
            (*num_definite)++;
        } else {
            /* Add to potential deadlocks */
            if (*num_potential >= potential_capacity) {
                int new_capacity = potential_capacity == 0 ? 10 : potential_capacity * 2;
                CycleInfo* new_list = (CycleInfo*)safe_realloc(
                    *potential_deadlocks, sizeof(CycleInfo) * new_capacity);
                if (new_list == NULL) {
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                *potential_deadlocks = new_list;
                potential_capacity = new_capacity;
            }
            
            /* Copy cycle structure */
            CycleInfo* dest = &(*potential_deadlocks)[*num_potential];
            memset(dest, 0, sizeof(CycleInfo));
            
            /* Copy cycle data */
            dest->cycle_length = cycles[i].cycle_length;
            dest->cycle_start_vertex = cycles[i].cycle_start_vertex;
            dest->cycle_end_vertex = cycles[i].cycle_end_vertex;
            dest->num_processes = cycles[i].num_processes;
            dest->num_resources = cycles[i].num_resources;
            
            /* Deep copy cycle_path */
            if (cycles[i].cycle_path != NULL && cycles[i].cycle_length > 0) {
                dest->cycle_path = (int*)safe_malloc(sizeof(int) * cycles[i].cycle_length);
                if (dest->cycle_path == NULL) {
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->cycle_path, cycles[i].cycle_path,
                       sizeof(int) * cycles[i].cycle_length);
            }
            
            /* Deep copy process_ids */
            if (cycles[i].process_ids != NULL && cycles[i].num_processes > 0) {
                dest->process_ids = (int*)safe_malloc(sizeof(int) * cycles[i].num_processes);
                if (dest->process_ids == NULL) {
                    free(dest->cycle_path);
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->process_ids, cycles[i].process_ids,
                       sizeof(int) * cycles[i].num_processes);
            }
            
            /* Deep copy resource_ids */
            if (cycles[i].resource_ids != NULL && cycles[i].num_resources > 0) {
                dest->resource_ids = (int*)safe_malloc(sizeof(int) * cycles[i].num_resources);
                if (dest->resource_ids == NULL) {
                    free(dest->cycle_path);
                    free(dest->process_ids);
                    free_cycle_list(*definite_deadlocks, *num_definite);
                    free_cycle_list(*potential_deadlocks, *num_potential);
                    return ERROR_OUT_OF_MEMORY;
                }
                memcpy(dest->resource_ids, cycles[i].resource_ids,
                       sizeof(int) * cycles[i].num_resources);
            }
            
            (*num_potential)++;
        }
    }
    
    return SUCCESS;
}

/*
 * analyze_cycles_for_deadlock - Analyze cycles to determine actual deadlocks
 * @cycles: Array of CycleInfo structures from cycle detection
 * @num_cycles: Number of cycles found
 * @graph: ResourceGraph that was analyzed
 * @report: Output parameter for deadlock report
 * @return: SUCCESS (0) on success, negative on error
 * Description: Analyzes each cycle to determine if it represents a real deadlock.
 *              For single-instance resources: cycle = definite deadlock
 *              For multi-instance resources: cycle = potential deadlock
 *              Extracts deadlocked process IDs and generates explanations.
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Returns error codes for allocation failures
 */
int analyze_cycles_for_deadlock(const CycleInfo* cycles, int num_cycles,
                               const ResourceGraph* graph, DeadlockReport* report)
{
    if (cycles == NULL || graph == NULL || report == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (num_cycles == 0) {
        report->deadlock_detected = 0;
        return SUCCESS;
    }
    
    /* Filter into definite and potential deadlocks */
    CycleInfo* definite_cycles = NULL;
    CycleInfo* potential_cycles = NULL;
    int num_definite = 0;
    int num_potential = 0;
    
    int filter_result = filter_actual_deadlocks(cycles, num_cycles, graph,
                                                &definite_cycles, &num_definite,
                                                &potential_cycles, &num_potential);
    
    if (filter_result != SUCCESS) {
        return filter_result;
    }
    
    /* Use definite deadlocks for report (or potential if no definite) */
    CycleInfo* deadlock_cycles = definite_cycles;
    int num_deadlock_cycles = num_definite;
    
    if (num_deadlock_cycles == 0 && num_potential > 0) {
        /* Use potential deadlocks if no definite ones */
        deadlock_cycles = potential_cycles;
        num_deadlock_cycles = num_potential;
    }
    
    /* Store cycles in report */
    report->cycles = deadlock_cycles;
    report->num_cycles = num_deadlock_cycles;
    
    if (num_deadlock_cycles > 0) {
        report->deadlock_detected = 1;
        
        /* Identify deadlocked processes */
        int result = identify_deadlocked_processes(deadlock_cycles, num_deadlock_cycles,
                                                  graph, &report->deadlocked_pids,
                                                  &report->num_deadlocked);
        if (result != SUCCESS) {
            /* Cleanup potential cycles that weren't used */
            if (num_potential > 0 && num_definite > 0) {
                free_cycle_list(potential_cycles, num_potential);
            }
            return result;
        }
    } else {
        report->deadlock_detected = 0;
    }
    
    /* Cleanup unused cycles */
    if (num_potential > 0 && num_definite > 0) {
        free_cycle_list(potential_cycles, num_potential);
    } else if (num_potential > 0 && num_definite == 0) {
        /* Potential cycles were used, free definite (should be NULL) */
        free_cycle_list(definite_cycles, num_definite);
    }
    
    return SUCCESS;
}

/*
 * generate_explanations - Generate human-readable explanations for deadlocks
 * @report: DeadlockReport to fill with explanations
 * @graph: ResourceGraph for context
 * @return: SUCCESS (0) on success, negative on error
 * Description: Creates detailed explanations describing why each deadlock occurred,
 *              including wait chains and resource dependencies.
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Returns error codes for allocation failures
 */
int generate_explanations(DeadlockReport* report, const ResourceGraph* graph)
{
    if (report == NULL || graph == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (report->num_cycles == 0) {
        return SUCCESS;
    }
    
    /* Allocate explanations array */
    report->explanations = (char**)safe_malloc(sizeof(char*) * report->num_cycles);
    if (report->explanations == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Generate explanation for each cycle */
    for (int i = 0; i < report->num_cycles; i++) {
        char explanation[1024];
        int pos = 0;
        
        pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                       "Cycle #%d: ", i + 1);
        
        if (report->cycles[i].cycle_path != NULL) {
            /* Build wait chain description */
            for (int j = 0; j < report->cycles[i].cycle_length - 1; j++) {
                int vertex = report->cycles[i].cycle_path[j];
                
                if (j > 0) {
                    pos += snprintf(explanation + pos, sizeof(explanation) - pos, " -> ");
                }
                
                if (vertex >= 0 && vertex < graph->num_vertices) {
                    if (graph->vertex_type[vertex] == VERTEX_TYPE_PROCESS) {
                        pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                                       "P%d", graph->vertex_id[vertex]);
                    } else {
                        pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                                       "R%d", graph->vertex_id[vertex]);
                    }
                }
            }
        }
        
        pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                       ". This cycle indicates a circular wait condition.");
        
        /* Determine if definite or potential */
        int is_definite = is_deadlock_definite(&report->cycles[i], graph);
        if (is_definite) {
            pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                           " This is a DEFINITE deadlock (single-instance resources).");
        } else {
            pos += snprintf(explanation + pos, sizeof(explanation) - pos,
                           " This is a POTENTIAL deadlock (multi-instance resources may resolve).");
        }
        
        /* Allocate and copy explanation */
        report->explanations[i] = str_dup(explanation);
        if (report->explanations[i] == NULL) {
            /* Free previous explanations */
            for (int k = 0; k < i; k++) {
                free(report->explanations[k]);
            }
            free(report->explanations);
            report->explanations = NULL;
            return ERROR_OUT_OF_MEMORY;
        }
    }
    
    report->num_explanations = report->num_cycles;
    return SUCCESS;
}

/*
 * generate_recommendations - Generate recommendations for resolving deadlocks
 * @report: DeadlockReport with detected deadlocks
 * @graph: ResourceGraph for context
 * @return: SUCCESS (0) on success, negative on error
 * Description: Analyzes deadlock situation and generates actionable recommendations
 *              such as which processes to kill, which resources to release, etc.
 *              Time complexity: O(D) where D is number of deadlocked processes
 * Error handling: Returns error codes for allocation failures
 */
int generate_recommendations(DeadlockReport* report, const ResourceGraph* graph)
{
    if (report == NULL || graph == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (report->num_deadlocked == 0) {
        return SUCCESS;
    }
    
    /* Allocate recommendations array */
    int max_recommendations = 5; /* Estimate */
    report->recommendations = (char**)safe_malloc(sizeof(char*) * max_recommendations);
    if (report->recommendations == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    int rec_count = 0;
    
    /* Recommendation 1: Terminate processes */
    if (report->num_deadlocked > 0) {
        char rec[512];
        int pos = snprintf(rec, sizeof(rec),
                          "Terminate one of the deadlocked processes: ");
        for (int i = 0; i < report->num_deadlocked && i < 10; i++) {
            if (i > 0) pos += snprintf(rec + pos, sizeof(rec) - pos, ", ");
            pos += snprintf(rec + pos, sizeof(rec) - pos, "PID %d",
                           report->deadlocked_pids[i]);
        }
        if (report->num_deadlocked > 10) {
            pos += snprintf(rec + pos, sizeof(rec) - pos, " (and %d more)",
                           report->num_deadlocked - 10);
        }
        pos += snprintf(rec + pos, sizeof(rec) - pos,
                       ". This will break the circular wait chain.");
        
        report->recommendations[rec_count] = str_dup(rec);
        if (report->recommendations[rec_count] == NULL) {
            goto cleanup;
        }
        rec_count++;
    }
    
    /* Recommendation 2: Resource release */
    if (rec_count < max_recommendations) {
        char* rec = "Review resource allocation policies to prevent circular dependencies.";
        report->recommendations[rec_count] = str_dup(rec);
        if (report->recommendations[rec_count] == NULL) {
            goto cleanup;
        }
        rec_count++;
    }
    
    /* Recommendation 3: Timeout */
    if (rec_count < max_recommendations) {
        char* rec = "Implement resource request timeouts to automatically break deadlocks.";
        report->recommendations[rec_count] = str_dup(rec);
        if (report->recommendations[rec_count] == NULL) {
            goto cleanup;
        }
        rec_count++;
    }
    
    report->num_recommendations = rec_count;
    return SUCCESS;
    
cleanup:
    for (int i = 0; i < rec_count; i++) {
        free(report->recommendations[i]);
    }
    free(report->recommendations);
    report->recommendations = NULL;
    report->num_recommendations = 0;
    return ERROR_OUT_OF_MEMORY;
}

/*
 * detect_deadlock_in_system - Main deadlock detection entry point
 * @procs: Array of ProcessResourceInfo structures for all processes
 * @num_procs: Number of processes in array
 * @report: Output parameter for deadlock report
 * @return: 1 if deadlock detected, 0 if no deadlock, negative on error
 * Description: Main function that orchestrates deadlock detection:
 *              1. Builds RAG from process resource information
 *              2. Runs cycle detection algorithm
 *              3. Analyzes cycles to determine actual deadlocks
 *              4. Generates comprehensive report
 *              Time complexity: O(V + E + C) where V=vertices, E=edges, C=cycles
 * Error handling: Returns negative error code on failure, fills report with
 *                 partial results if possible
 */
int detect_deadlock_in_system(ProcessResourceInfo* procs, int num_procs,
                              DeadlockReport* report)
{
    if (procs == NULL || report == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (num_procs <= 0) {
        report->deadlock_detected = 0;
        report->total_processes_scanned = 0;
        return 0;
    }
    
    /* Initialize report */
    report->deadlock_detected = 0;
    report->total_processes_scanned = num_procs;
    
    /* Step 1: Build Resource Allocation Graph */
    ResourceGraph* graph = NULL;
    int build_result = build_rag_from_processes(procs, num_procs, &graph);
    if (build_result != SUCCESS) {
        error_log("Failed to build RAG: %d", build_result);
        return build_result;
    }
    
    if (graph == NULL) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Get resource count for statistics */
    int num_processes, num_resources, num_edges;
    get_graph_statistics(graph, &num_processes, &num_resources, &num_edges);
    report->total_resources_found = num_resources;
    
    /* Step 2: Run cycle detection */
    CycleInfo* cycles = NULL;
    int num_cycles = 0;
    
    reset_graph_colors(graph);
    int cycle_result = find_all_cycles(graph, &cycles, &num_cycles);
    
    if (cycle_result != SUCCESS) {
        error_log("Cycle detection failed: %d", cycle_result);
        free_graph(graph);
        return cycle_result;
    }
    
    /* Step 3: Analyze cycles for deadlocks */
    if (num_cycles > 0) {
        int analyze_result = analyze_cycles_for_deadlock(cycles, num_cycles, graph, report);
        if (analyze_result != SUCCESS) {
            error_log("Cycle analysis failed: %d", analyze_result);
            free_cycle_list(cycles, num_cycles);
            free_graph(graph);
            return analyze_result;
        }
        
        /* Free original cycles list (analyze_cycles_for_deadlock creates copies) */
        free_cycle_list(cycles, num_cycles);
        cycles = NULL;
        num_cycles = 0;
    } else {
        report->deadlock_detected = 0;
    }
    
    /* Step 4: Generate explanations and recommendations */
    if (report->deadlock_detected) {
        int explain_result = generate_explanations(report, graph);
        if (explain_result != SUCCESS) {
            debug_log("Failed to generate explanations: %d", explain_result);
            /* Non-fatal, continue */
        }
        
        int rec_result = generate_recommendations(report, graph);
        if (rec_result != SUCCESS) {
            debug_log("Failed to generate recommendations: %d", rec_result);
            /* Non-fatal, continue */
        }
    }
    
    /* Cleanup graph */
    free_graph(graph);
    
    return report->deadlock_detected ? 1 : 0;
}

/*
 * free_deadlock_report - Free all memory allocated for DeadlockReport
 * @report: DeadlockReport to free
 * @return: None
 * Description: Frees all dynamically allocated arrays and strings in report,
 *              including cycles, PIDs, explanations, and recommendations.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer and partially initialized reports safely
 */
void free_deadlock_report(DeadlockReport* report)
{
    if (report == NULL) {
        return;
    }
    
    /* Free deadlocked PIDs array */
    safe_free((void**)&report->deadlocked_pids);
    
    /* Free cycles */
    if (report->cycles != NULL) {
        free_cycle_list(report->cycles, report->num_cycles);
        report->cycles = NULL;
    }
    
    /* Free explanations */
    if (report->explanations != NULL) {
        for (int i = 0; i < report->num_explanations; i++) {
            if (report->explanations[i] != NULL) {
                free(report->explanations[i]);
            }
        }
        free(report->explanations);
        report->explanations = NULL;
    }
    
    /* Free recommendations */
    if (report->recommendations != NULL) {
        for (int i = 0; i < report->num_recommendations; i++) {
            if (report->recommendations[i] != NULL) {
                free(report->recommendations[i]);
            }
        }
        free(report->recommendations);
        report->recommendations = NULL;
    }
    
    /* Reset counters */
    report->num_deadlocked = 0;
    report->num_cycles = 0;
    report->num_explanations = 0;
    report->num_recommendations = 0;
}

/*
 * analyze_pipe_and_lock_dependencies - Analyze pipe and lock dependencies
 * @procs: Array of ProcessResourceInfo structures
 * @num_procs: Number of processes
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Analyzes system-wide locks and pipe relationships to determine
 *              which processes are waiting on which resources/processes.
 *              Updates ProcessResourceInfo structures with waiting resources
 *              and waiting_on_pids. This enables detection of pipe and lock deadlocks.
 *              Time complexity: O(P * L + P * F) where P=processes, L=locks, F=FDs
 * Error handling: Returns error codes for allocation or access issues
 */
int analyze_pipe_and_lock_dependencies(ProcessResourceInfo* procs, int num_procs)
{
    if (procs == NULL || num_procs <= 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Step 1: Parse system-wide locks */
    FileLockInfo* system_locks = NULL;
    int system_lock_count = 0;
    int lock_result = parse_system_locks(&system_locks, &system_lock_count);
    
    if (lock_result != SUCCESS && lock_result != ERROR_FILE_NOT_FOUND) {
        /* Non-fatal, continue without lock analysis */
        debug_log("Failed to parse system locks: %d", lock_result);
    }
    
    /* Step 2: Build PID to ProcessResourceInfo map for quick lookup */
    /* We'll iterate through procs array directly */
    
    /* Step 3: Analyze pipe dependencies */
    /* For each process with pipes, check pipe relationships */
    for (int i = 0; i < num_procs; i++) {
        ProcessResourceInfo* proc = &procs[i];
        
        /* Check if process has pipes (blocked or not, both can lead to deadlock) */
        if (proc->num_pipe_inodes > 0) {
            /* Find processes that share the same pipe inode */
            for (int j = 0; j < num_procs; j++) {
                if (i == j) {
                    continue;
                }
                
                ProcessResourceInfo* other_proc = &procs[j];
                
                /* Check if other process has matching pipe inode */
                for (int k = 0; k < proc->num_pipe_inodes; k++) {
                    unsigned long pipe_inode = proc->pipe_inodes[k];
                    
                    for (int l = 0; l < other_proc->num_pipe_inodes; l++) {
                        if (other_proc->pipe_inodes[l] == pipe_inode) {
                            /* Found pipe relationship! */
                            /* Create resource ID from pipe inode */
                            int pipe_resource_id = (int)(pipe_inode % 1000000); /* Use last 6 digits as resource ID */
                            
                            /* If proc is blocked on pipe, it's waiting for the pipe resource */
                            if (proc->is_blocked_on_pipe) {
                                /* Add other_proc->pid to waiting_on_pids */
                                if (proc->waiting_on_pids == NULL) {
                                    proc->waiting_on_pids = (int*)safe_malloc(sizeof(int) * MAX_WAITING_PIDS);
                                    proc->num_waiting_on_pids = 0;
                                }
                                
                                if (proc->num_waiting_on_pids < MAX_WAITING_PIDS) {
                                    /* Check if PID already in array */
                                    int found = 0;
                                    for (int m = 0; m < proc->num_waiting_on_pids; m++) {
                                        if (proc->waiting_on_pids[m] == other_proc->pid) {
                                            found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!found) {
                                        proc->waiting_on_pids[proc->num_waiting_on_pids++] = other_proc->pid;
                                    }
                                }
                                
                                /* Add to waiting resources */
                                if (proc->waiting_resources == NULL) {
                                    proc->waiting_resources = (int*)safe_malloc(sizeof(int) * MAX_RESOURCES_PER_PROCESS);
                                    proc->num_waiting = 0;
                                }
                                
                                if (proc->num_waiting < MAX_RESOURCES_PER_PROCESS) {
                                    /* Check if resource already in array */
                                    int res_found = 0;
                                    for (int m = 0; m < proc->num_waiting; m++) {
                                        if (proc->waiting_resources[m] == pipe_resource_id) {
                                            res_found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!res_found) {
                                        proc->waiting_resources[proc->num_waiting++] = pipe_resource_id;
                                    }
                                }
                            }
                            
                            /* other_proc holds the pipe (has the other end) */
                            /* Add pipe as held resource for other_proc */
                            if (other_proc->held_resources == NULL) {
                                other_proc->held_resources = (int*)safe_malloc(sizeof(int) * MAX_RESOURCES_PER_PROCESS);
                                other_proc->num_held = 0;
                            }
                            
                            if (other_proc->num_held < MAX_RESOURCES_PER_PROCESS) {
                                int held_found = 0;
                                for (int m = 0; m < other_proc->num_held; m++) {
                                    if (other_proc->held_resources[m] == pipe_resource_id) {
                                        held_found = 1;
                                        break;
                                    }
                                }
                                
                                if (!held_found) {
                                    other_proc->held_resources[other_proc->num_held++] = pipe_resource_id;
                                }
                            }
                            
                            /* If other_proc is also blocked on the same pipe, it creates mutual dependency */
                            if (other_proc->is_blocked_on_pipe) {
                                /* Reverse relationship: other_proc is waiting for proc */
                                if (other_proc->waiting_on_pids == NULL) {
                                    other_proc->waiting_on_pids = (int*)safe_malloc(sizeof(int) * MAX_WAITING_PIDS);
                                    other_proc->num_waiting_on_pids = 0;
                                }
                                
                                if (other_proc->num_waiting_on_pids < MAX_WAITING_PIDS) {
                                    int found = 0;
                                    for (int m = 0; m < other_proc->num_waiting_on_pids; m++) {
                                        if (other_proc->waiting_on_pids[m] == proc->pid) {
                                            found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!found) {
                                        other_proc->waiting_on_pids[other_proc->num_waiting_on_pids++] = proc->pid;
                                    }
                                }
                                
                                /* Add to waiting resources for other_proc */
                                if (other_proc->waiting_resources == NULL) {
                                    other_proc->waiting_resources = (int*)safe_malloc(sizeof(int) * MAX_RESOURCES_PER_PROCESS);
                                    other_proc->num_waiting = 0;
                                }
                                
                                if (other_proc->num_waiting < MAX_RESOURCES_PER_PROCESS) {
                                    int res_found = 0;
                                    for (int m = 0; m < other_proc->num_waiting; m++) {
                                        if (other_proc->waiting_resources[m] == pipe_resource_id) {
                                            res_found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!res_found) {
                                        other_proc->waiting_resources[other_proc->num_waiting++] = pipe_resource_id;
                                    }
                                }
                                
                                /* proc also holds the pipe */
                                if (proc->held_resources == NULL) {
                                    proc->held_resources = (int*)safe_malloc(sizeof(int) * MAX_RESOURCES_PER_PROCESS);
                                    proc->num_held = 0;
                                }
                                
                                if (proc->num_held < MAX_RESOURCES_PER_PROCESS) {
                                    int held_found = 0;
                                    for (int m = 0; m < proc->num_held; m++) {
                                        if (proc->held_resources[m] == pipe_resource_id) {
                                            held_found = 1;
                                            break;
                                        }
                                    }
                                    
                                    if (!held_found) {
                                        proc->held_resources[proc->num_held++] = pipe_resource_id;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        
        /* Step 4: Analyze file lock dependencies */
        if (proc->is_blocked_on_lock && system_locks != NULL && system_lock_count > 0) {
            /* Find locks that this process is waiting for */
            for (int j = 0; j < system_lock_count; j++) {
                FileLockInfo* lock = &system_locks[j];
                
                /* Check if this lock is blocking (WRITE lock) */
                if (lock->is_blocking && lock->pid != proc->pid) {
                    /* Check if process is blocked waiting for this lock */
                    /* We need to match based on inode or file path */
                    /* For now, add all blocking locks as waiting resources */
                    
                    int lock_resource_id = lock->lock_id;
                    
                    /* Add to waiting resources */
                    if (proc->waiting_resources == NULL) {
                        proc->waiting_resources = (int*)safe_malloc(sizeof(int) * MAX_RESOURCES_PER_PROCESS);
                        proc->num_waiting = 0;
                    }
                    
                    if (proc->num_waiting < MAX_RESOURCES_PER_PROCESS) {
                        /* Check if resource already in array */
                        int res_found = 0;
                        for (int k = 0; k < proc->num_waiting; k++) {
                            if (proc->waiting_resources[k] == lock_resource_id) {
                                res_found = 1;
                                break;
                            }
                        }
                        
                        if (!res_found) {
                            proc->waiting_resources[proc->num_waiting++] = lock_resource_id;
                            
                            /* Find process holding the lock and add to waiting_on_pids */
                            for (int k = 0; k < num_procs; k++) {
                                if (procs[k].pid == lock->pid) {
                                    /* Add lock->pid to waiting_on_pids */
                                    if (proc->waiting_on_pids == NULL) {
                                        proc->waiting_on_pids = (int*)safe_malloc(sizeof(int) * MAX_WAITING_PIDS);
                                        proc->num_waiting_on_pids = 0;
                                    }
                                    
                                    if (proc->num_waiting_on_pids < MAX_WAITING_PIDS) {
                                        int pid_found = 0;
                                        for (int m = 0; m < proc->num_waiting_on_pids; m++) {
                                            if (proc->waiting_on_pids[m] == lock->pid) {
                                                pid_found = 1;
                                                break;
                                            }
                                        }
                                        
                                        if (!pid_found) {
                                            proc->waiting_on_pids[proc->num_waiting_on_pids++] = lock->pid;
                                        }
                                    }
                                    break;
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    
    /* Cleanup */
    if (system_locks != NULL) {
        free_file_lock_info(system_locks, system_lock_count);
    }
    
    return SUCCESS;
}

