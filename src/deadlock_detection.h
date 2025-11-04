#ifndef DEADLOCK_DETECTION_H
#define DEADLOCK_DETECTION_H

/* =============================================================================
 * DEADLOCK_DETECTION.H - Deadlock Detection Engine Interface
 * =============================================================================
 * This header defines structures and functions for the main deadlock detection
 * logic that integrates process monitoring, graph building, and cycle detection.
 * =============================================================================
 */

#include "cycle_detection.h"
#include "process_monitor.h"
#include "config.h"

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

/*
 * DeadlockReport - Comprehensive deadlock detection report
 * Contains all information about detected deadlocks
 */
typedef struct {
    int deadlock_detected;          /* 1 if deadlock found, 0 otherwise */
    int* deadlocked_pids;            /* Array of process IDs involved in deadlock */
    int num_deadlocked;             /* Number of deadlocked processes */
    CycleInfo* cycles;               /* Array of cycles detected */
    int num_cycles;                  /* Number of cycles found */
    char** explanations;             /* Array of explanation strings for each deadlock */
    int num_explanations;            /* Number of explanations */
    char** recommendations;          /* Array of recommendation strings */
    int num_recommendations;         /* Number of recommendations */
    int timestamp;                   /* Detection timestamp (Unix time) */
    int total_processes_scanned;     /* Total number of processes analyzed */
    int total_resources_found;       /* Total number of resources found */
} DeadlockReport;

/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

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
                              DeadlockReport* report);

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
                             ResourceGraph** graph);

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
                               const ResourceGraph* graph, DeadlockReport* report);

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
                                  const ResourceGraph* graph, int** pids, int* count);

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
                           CycleInfo** potential_deadlocks, int* num_potential);

/*
 * create_deadlock_report - Allocate and initialize a new DeadlockReport
 * @return: Pointer to allocated DeadlockReport, or NULL on failure
 * Description: Creates a new DeadlockReport structure with all fields initialized.
 *              Time complexity: O(1)
 * Error handling: Returns NULL on allocation failure
 */
DeadlockReport* create_deadlock_report(void);

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
int generate_recommendations(DeadlockReport* report, const ResourceGraph* graph);

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
int generate_explanations(DeadlockReport* report, const ResourceGraph* graph);

/*
 * free_deadlock_report - Free all memory allocated for DeadlockReport
 * @report: DeadlockReport to free
 * @return: None
 * Description: Frees all dynamically allocated arrays and strings in report,
 *              including cycles, PIDs, explanations, and recommendations.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer and partially initialized reports safely
 */
void free_deadlock_report(DeadlockReport* report);

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
int is_deadlock_definite(const CycleInfo* cycle, const ResourceGraph* graph);

#endif /* DEADLOCK_DETECTION_H */

