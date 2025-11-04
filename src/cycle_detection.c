/* =============================================================================
 * CYCLE_DETECTION.C - Cycle Detection Implementation
 * =============================================================================
 * Implementation of DFS-based cycle detection algorithm using 3-color marking
 * to detect all cycles in Resource Allocation Graphs.
 * =============================================================================
 */

#include "cycle_detection.h"
#include "utility.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* =============================================================================
 * HELPER FUNCTIONS
 * =============================================================================
 */

/*
 * is_duplicate_cycle - Check if a cycle is duplicate of existing ones
 * @cycle: Cycle to check
 * @cycle_list: Array of existing cycles
 * @num_cycles: Number of existing cycles
 * @return: 1 if duplicate, 0 otherwise
 */
static int is_duplicate_cycle(const CycleInfo* cycle, 
                              const CycleInfo* cycle_list, 
                              int num_cycles)
{
    if (cycle == NULL || cycle_list == NULL || num_cycles <= 0) {
        return 0;
    }
    
    for (int i = 0; i < num_cycles; i++) {
        const CycleInfo* existing = &cycle_list[i];
        
        if (existing->cycle_length != cycle->cycle_length) {
            continue;
        }
        
        /* Check if cycles have same vertices (rotated) */
        int match = 1;
        for (int offset = 0; offset < cycle->cycle_length; offset++) {
            match = 1;
            for (int j = 0; j < cycle->cycle_length; j++) {
                int idx1 = (j + offset) % cycle->cycle_length;
                int idx2 = j;
                if (existing->cycle_path[idx1] != cycle->cycle_path[idx2]) {
                    match = 0;
                    break;
                }
            }
            if (match) {
                return 1;
            }
        }
    }
    
    return 0;
}

/*
 * add_cycle_to_list - Add cycle to list, expanding if needed
 * @cycle_list: Pointer to cycle list array
 * @num_cycles: Pointer to current count
 * @capacity: Pointer to current capacity
 * @cycle: Cycle to add
 * @return: SUCCESS (0) on success, negative on error
 */
static int add_cycle_to_list(CycleInfo** cycle_list, int* num_cycles, 
                             int* capacity, const CycleInfo* cycle)
{
    if (cycle_list == NULL || num_cycles == NULL || capacity == NULL || 
        cycle == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Check for duplicates */
    if (is_duplicate_cycle(cycle, *cycle_list, *num_cycles)) {
        return SUCCESS; /* Skip duplicate */
    }
    
    /* Expand array if needed */
    if (*num_cycles >= *capacity) {
        int new_capacity = *capacity == 0 ? 10 : *capacity * 2;
        CycleInfo* new_list = (CycleInfo*)safe_realloc(
            *cycle_list, sizeof(CycleInfo) * new_capacity);
        if (new_list == NULL) {
            return ERROR_OUT_OF_MEMORY;
        }
        *cycle_list = new_list;
        *capacity = new_capacity;
    }
    
    /* Copy cycle info */
    CycleInfo* dest = &(*cycle_list)[*num_cycles];
    memset(dest, 0, sizeof(CycleInfo));
    
    dest->cycle_length = cycle->cycle_length;
    dest->cycle_start_vertex = cycle->cycle_start_vertex;
    dest->cycle_end_vertex = cycle->cycle_end_vertex;
    
    /* Allocate and copy cycle path */
    if (cycle->cycle_path != NULL && cycle->cycle_length > 0) {
        dest->cycle_path = (int*)safe_malloc(sizeof(int) * cycle->cycle_length);
        if (dest->cycle_path == NULL) {
            return ERROR_OUT_OF_MEMORY;
        }
        memcpy(dest->cycle_path, cycle->cycle_path, 
               sizeof(int) * cycle->cycle_length);
    }
    
    /* Copy process and resource IDs if available */
    if (cycle->process_ids != NULL && cycle->num_processes > 0) {
        dest->process_ids = (int*)safe_malloc(sizeof(int) * cycle->num_processes);
        if (dest->process_ids == NULL) {
            free(dest->cycle_path);
            return ERROR_OUT_OF_MEMORY;
        }
        memcpy(dest->process_ids, cycle->process_ids, 
               sizeof(int) * cycle->num_processes);
        dest->num_processes = cycle->num_processes;
    }
    
    if (cycle->resource_ids != NULL && cycle->num_resources > 0) {
        dest->resource_ids = (int*)safe_malloc(sizeof(int) * cycle->num_resources);
        if (dest->resource_ids == NULL) {
            free(dest->cycle_path);
            free(dest->process_ids);
            return ERROR_OUT_OF_MEMORY;
        }
        memcpy(dest->resource_ids, cycle->resource_ids, 
               sizeof(int) * cycle->num_resources);
        dest->num_resources = cycle->num_resources;
    }
    
    (*num_cycles)++;
    return SUCCESS;
}

/* =============================================================================
 * MAIN FUNCTIONS
 * =============================================================================
 */

/*
 * detect_back_edge - Detect if an edge is a back edge (forms a cycle)
 * @graph: ResourceGraph being analyzed
 * @from_vertex: Source vertex of edge
 * @to_vertex: Destination vertex of edge
 * @color: Current color array
 * @return: 1 if back edge, 0 otherwise
 * Description: Checks if edge from_vertex->to_vertex is a back edge.
 *              Back edge exists when to_vertex is GRAY (in recursion stack).
 *              Time complexity: O(1)
 * Error handling: Returns 0 for invalid parameters
 */
int detect_back_edge(const ResourceGraph* graph, int from_vertex, int to_vertex,
                     const int* color)
{
    if (graph == NULL || color == NULL) {
        return 0;
    }
    
    if (from_vertex < 0 || from_vertex >= graph->num_vertices ||
        to_vertex < 0 || to_vertex >= graph->num_vertices) {
        return 0;
    }
    
    /* Back edge: edge to a GRAY vertex (in recursion stack) */
    return (color[to_vertex] == COLOR_GRAY) ? 1 : 0;
}

/*
 * extract_cycle_path - Reconstruct cycle path from parent array
 * @parent: Parent array from DFS traversal
 * @ancestor: Ancestor vertex (GRAY, the target of back edge)
 * @current: Current vertex (source of back edge)
 * @graph: ResourceGraph for vertex information
 * @cycle_info: Output structure to fill with cycle path
 * @return: SUCCESS (0) on success, negative on error
 * Description: Reconstructs the complete cycle path when back edge is detected.
 *              When current->ancestor is a back edge, ancestor is in parent chain of current.
 *              Cycle: ancestor -> ... -> current -> ancestor
 *              Time complexity: O(cycle_length)
 * Error handling: Returns error codes for allocation failures
 */
int extract_cycle_path(const int* parent, int ancestor, int current,
                       const ResourceGraph* graph, CycleInfo* cycle_info)
{
    if (parent == NULL || graph == NULL || cycle_info == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (ancestor < 0 || ancestor >= graph->num_vertices ||
        current < 0 || current >= graph->num_vertices) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Handle self-loop: current == ancestor */
    if (current == ancestor) {
        /* Self-loop: ancestor -> ancestor */
        cycle_info->cycle_path = (int*)safe_malloc(sizeof(int) * 2);
        if (cycle_info->cycle_path == NULL) {
            return ERROR_OUT_OF_MEMORY;
        }
        cycle_info->cycle_path[0] = ancestor;
        cycle_info->cycle_path[1] = ancestor;
        cycle_info->cycle_length = 2;
        cycle_info->cycle_start_vertex = ancestor;
        cycle_info->cycle_end_vertex = ancestor;
        
        /* Extract process/resource IDs */
        if (graph->vertex_type[ancestor] == VERTEX_TYPE_PROCESS) {
            cycle_info->process_ids = (int*)safe_malloc(sizeof(int) * 1);
            if (cycle_info->process_ids != NULL) {
                cycle_info->process_ids[0] = graph->vertex_id[ancestor];
                cycle_info->num_processes = 1;
            }
        } else if (graph->vertex_type[ancestor] == VERTEX_TYPE_RESOURCE) {
            cycle_info->resource_ids = (int*)safe_malloc(sizeof(int) * 1);
            if (cycle_info->resource_ids != NULL) {
                cycle_info->resource_ids[0] = graph->vertex_id[ancestor];
                cycle_info->num_resources = 1;
            }
        }
        
        return SUCCESS;
    }
    
    /* When back edge current->ancestor is detected:
     * - ancestor is GRAY (in recursion stack)
     * - ancestor is an ancestor of current in DFS tree
     * - Parent chain from current leads back to ancestor
     * - Cycle: ancestor -> ... -> current -> ancestor
     */
    
    /* Build path from current back to ancestor by following parent chain */
    int* temp_path = (int*)safe_malloc(sizeof(int) * graph->num_vertices);
    if (temp_path == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    int path_idx = 0;
    int v = current;
    
    /* Follow parent chain from current back to ancestor */
    while (v != ancestor && v >= 0 && path_idx < graph->num_vertices) {
        temp_path[path_idx++] = v;
        v = parent[v];
        if (v < 0) {
            /* Parent chain broken - this shouldn't happen with valid back edge */
            free(temp_path);
            return ERROR_INVALID_ARGUMENT;
        }
    }
    
    if (v != ancestor) {
        /* Couldn't reach ancestor - invalid back edge */
        free(temp_path);
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Add ancestor at the end */
    temp_path[path_idx++] = ancestor;
    
    /* Path length: ancestor -> ... -> current -> ancestor */
    int path_length = path_idx + 1; /* +1 to close cycle with ancestor again */
    
    /* Allocate cycle path array */
    cycle_info->cycle_path = (int*)safe_malloc(sizeof(int) * path_length);
    if (cycle_info->cycle_path == NULL) {
        free(temp_path);
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Build cycle: start from ancestor, follow path to current, then back to ancestor */
    int pos = 0;
    cycle_info->cycle_path[pos++] = ancestor; /* Start with ancestor */
    
    /* Reverse temp_path to get: ancestor -> ... -> current */
    for (int i = path_idx - 2; i >= 0; i--) { /* Skip last (ancestor) in temp_path */
        cycle_info->cycle_path[pos++] = temp_path[i];
    }
    
    cycle_info->cycle_path[pos++] = ancestor; /* Close cycle */
    
    cycle_info->cycle_length = path_length;
    cycle_info->cycle_start_vertex = ancestor;
    cycle_info->cycle_end_vertex = ancestor; /* Cycle is closed */
    
    free(temp_path);
    
    /* Extract process and resource IDs (excluding duplicate closing vertex) */
    int num_processes = 0;
    int num_resources = 0;
    
    /* Count unique vertices (exclude last duplicate) */
    for (int i = 0; i < path_length - 1; i++) {
        int vertex = cycle_info->cycle_path[i];
        if (vertex >= 0 && vertex < graph->num_vertices) {
            if (graph->vertex_type[vertex] == VERTEX_TYPE_PROCESS) {
                num_processes++;
            } else if (graph->vertex_type[vertex] == VERTEX_TYPE_RESOURCE) {
                num_resources++;
            }
        }
    }
    
    if (num_processes > 0) {
        cycle_info->process_ids = (int*)safe_malloc(sizeof(int) * num_processes);
        if (cycle_info->process_ids == NULL) {
            free(cycle_info->cycle_path);
            cycle_info->cycle_path = NULL;
            return ERROR_OUT_OF_MEMORY;
        }
        
        int proc_idx = 0;
        for (int i = 0; i < path_length - 1; i++) {
            int vertex = cycle_info->cycle_path[i];
            if (vertex >= 0 && vertex < graph->num_vertices &&
                graph->vertex_type[vertex] == VERTEX_TYPE_PROCESS) {
                cycle_info->process_ids[proc_idx++] = graph->vertex_id[vertex];
            }
        }
        cycle_info->num_processes = num_processes;
    }
    
    if (num_resources > 0) {
        cycle_info->resource_ids = (int*)safe_malloc(sizeof(int) * num_resources);
        if (cycle_info->resource_ids == NULL) {
            free(cycle_info->cycle_path);
            free(cycle_info->process_ids);
            cycle_info->cycle_path = NULL;
            cycle_info->process_ids = NULL;
            return ERROR_OUT_OF_MEMORY;
        }
        
        int res_idx = 0;
        for (int i = 0; i < path_length - 1; i++) {
            int vertex = cycle_info->cycle_path[i];
            if (vertex >= 0 && vertex < graph->num_vertices &&
                graph->vertex_type[vertex] == VERTEX_TYPE_RESOURCE) {
                cycle_info->resource_ids[res_idx++] = graph->vertex_id[vertex];
            }
        }
        cycle_info->num_resources = num_resources;
    }
    
    return SUCCESS;
}

/*
 * dfs_visit - Recursive DFS visit for cycle detection
 * @graph: ResourceGraph being traversed
 * @vertex: Current vertex being visited
 * @color: Color array for DFS marking
 * @parent: Parent array for path reconstruction
 * @cycle_list: Output list for detected cycles
 * @num_cycles: Pointer to cycle count
 * @capacity: Pointer to cycle list capacity
 * @return: SUCCESS (0) on success, negative on error
 * Description: Recursive DFS function that marks vertices and detects back edges.
 *              When a back edge (edge to GRAY vertex) is found, a cycle is detected.
 *              Time complexity: O(1) per call, O(V+E) total
 * Error handling: Returns error codes for invalid parameters
 */
static int dfs_visit_recursive(ResourceGraph* graph, int vertex, int* color, 
                               int* parent, CycleInfo** cycle_list,
                               int* num_cycles, int* capacity)
{
    if (graph == NULL || color == NULL || parent == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (vertex < 0 || vertex >= graph->num_vertices) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Mark vertex as GRAY (in recursion stack) */
    color[vertex] = COLOR_GRAY;
    
    /* Visit all neighbors */
    GraphNode* current = graph->adjacency_list[vertex];
    while (current != NULL) {
        int neighbor = current->vertex_id;
        
        if (neighbor < 0 || neighbor >= graph->num_vertices) {
            current = current->next;
            continue;
        }
        
        if (color[neighbor] == COLOR_WHITE) {
            /* Unvisited vertex - recurse */
            parent[neighbor] = vertex;
            int result = dfs_visit_recursive(graph, neighbor, color, parent,
                                            cycle_list, num_cycles, capacity);
            if (result != SUCCESS) {
                return result;
            }
        } else if (color[neighbor] == COLOR_GRAY) {
            /* Back edge detected - cycle found! */
            /* neighbor is GRAY (ancestor), vertex is current */
            /* Cycle: neighbor -> ... -> vertex -> neighbor */
            CycleInfo cycle;
            memset(&cycle, 0, sizeof(CycleInfo));
            
            int result = extract_cycle_path(parent, neighbor, vertex, graph, &cycle);
            if (result == SUCCESS) {
                /* Add cycle to list */
                add_cycle_to_list(cycle_list, num_cycles, capacity, &cycle);
                
                /* Free temporary cycle structure */
                free_cycle_info(&cycle);
            }
        }
        /* If BLACK, skip (already processed) */
        
        current = current->next;
    }
    
    /* Mark vertex as BLACK (finished) */
    color[vertex] = COLOR_BLACK;
    
    return SUCCESS;
}

/*
 * dfs_visit_single - Internal recursive DFS for single cycle detection
 */
static int dfs_visit_single(ResourceGraph* graph, int vertex, int* color, 
                           int* parent, CycleInfo* cycle_info, int* found)
{
    if (graph == NULL || color == NULL || parent == NULL || 
        cycle_info == NULL || found == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (vertex < 0 || vertex >= graph->num_vertices) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (*found) {
        return SUCCESS; /* Already found a cycle */
    }
    
    /* Mark vertex as GRAY (in recursion stack) */
    color[vertex] = COLOR_GRAY;
    
    /* Visit all neighbors */
    GraphNode* current = graph->adjacency_list[vertex];
    while (current != NULL && !(*found)) {
        int neighbor = current->vertex_id;
        
        if (neighbor >= 0 && neighbor < graph->num_vertices) {
            if (color[neighbor] == COLOR_WHITE) {
                /* Unvisited vertex - recurse */
                parent[neighbor] = vertex;
                int result = dfs_visit_single(graph, neighbor, color, parent,
                                              cycle_info, found);
                if (result != SUCCESS) {
                    return result;
                }
            } else if (color[neighbor] == COLOR_GRAY) {
                /* Back edge detected - cycle found! */
                /* neighbor is GRAY (ancestor), vertex is current */
                int result = extract_cycle_path(parent, neighbor, vertex, graph, cycle_info);
                if (result == SUCCESS) {
                    *found = 1;
                    return SUCCESS;
                }
            }
        }
        current = current->next;
    }
    
    /* Mark vertex as BLACK (finished) */
    color[vertex] = COLOR_BLACK;
    
    return SUCCESS;
}

/*
 * dfs_visit - Recursive DFS visit for cycle detection (wrapper)
 * @graph: ResourceGraph being traversed
 * @vertex: Current vertex being visited
 * @color: Color array for DFS marking
 * @parent: Parent array for path reconstruction
 * @cycle_info: Output structure for detected cycle (if found)
 * @found: Flag indicating if cycle was found
 * @return: SUCCESS (0) on success, negative on error
 * Description: Wrapper function that finds one cycle starting from vertex.
 *              Note: For finding all cycles, use find_all_cycles() instead.
 */
int dfs_visit(ResourceGraph* graph, int vertex, int* color, int* parent,
              CycleInfo* cycle_info, int* found)
{
    if (graph == NULL || color == NULL || parent == NULL || 
        cycle_info == NULL || found == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *found = 0;
    memset(cycle_info, 0, sizeof(CycleInfo));
    
    if (vertex < 0 || vertex >= graph->num_vertices) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    return dfs_visit_single(graph, vertex, color, parent, cycle_info, found);
}

/*
 * find_all_cycles - Find all cycles in the graph
 * @graph: ResourceGraph to analyze
 * @cycle_list: Output parameter for array of all cycles
 * @num_cycles: Output parameter for number of cycles found
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Finds ALL cycles in graph, not just the first one.
 *              Each cycle is stored as a CycleInfo structure.
 *              Time complexity: O(V + E)
 *              Space complexity: O(V + C) where C is number of cycles
 * Error handling: Returns error codes for allocation failures
 */
int find_all_cycles(ResourceGraph* graph, CycleInfo** cycle_list, int* num_cycles)
{
    if (graph == NULL || cycle_list == NULL || num_cycles == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *cycle_list = NULL;
    *num_cycles = 0;
    
    /* Reset graph colors */
    reset_graph_colors(graph);
    
    /* Initialize cycle list */
    int capacity = 0;
    CycleInfo* cycles = NULL;
    
    /* Perform DFS from each unvisited vertex */
    for (int i = 0; i < graph->num_vertices; i++) {
        if (graph->color[i] == COLOR_WHITE) {
            /* Reset parent array for this DFS tree */
            for (int j = 0; j < graph->num_vertices; j++) {
                graph->parent[j] = -1;
            }
            
            int result = dfs_visit_recursive(graph, i, graph->color, graph->parent,
                                            &cycles, num_cycles, &capacity);
            if (result != SUCCESS) {
                free_cycle_list(cycles, *num_cycles);
                *cycle_list = NULL;
                *num_cycles = 0;
                return result;
            }
        }
    }
    
    *cycle_list = cycles;
    return SUCCESS;
}

/*
 * has_cycle - Detect if graph contains any cycles
 * @graph: ResourceGraph to analyze
 * @cycle_list: Output parameter for array of detected cycles
 * @num_cycles: Output parameter for number of cycles found
 * @return: 1 if cycles exist, 0 if no cycles, negative on error
 * Description: Main entry point for cycle detection. Uses DFS to find all
 *              cycles in the graph. Returns array of CycleInfo structures.
 *              Time complexity: O(V + E) where V=vertices, E=edges
 *              Space complexity: O(V)
 * Error handling: Returns negative error code on failure, sets num_cycles to 0
 */
int has_cycle(ResourceGraph* graph, CycleInfo** cycle_list, int* num_cycles)
{
    if (graph == NULL || cycle_list == NULL || num_cycles == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    int result = find_all_cycles(graph, cycle_list, num_cycles);
    if (result != SUCCESS) {
        *cycle_list = NULL;
        *num_cycles = 0;
        return result;
    }
    
    return (*num_cycles > 0) ? 1 : 0;
}

/*
 * print_cycle - Print cycle information in readable format
 * @cycle: CycleInfo structure to print
 * @graph: ResourceGraph for vertex information
 * @return: None
 * Description: Prints cycle as sequence of vertices: P1->R1->P2->R2->P1
 *              Shows both vertex indices and actual PIDs/RIDs.
 *              Time complexity: O(cycle_length)
 * Error handling: Handles NULL pointers safely
 */
void print_cycle(const CycleInfo* cycle, const ResourceGraph* graph)
{
    if (cycle == NULL || graph == NULL) {
        printf("Invalid cycle or graph\n");
        return;
    }
    
    if (cycle->cycle_path == NULL || cycle->cycle_length == 0) {
        printf("Empty cycle\n");
        return;
    }
    
    printf("Cycle (length=%d): ", cycle->cycle_length);
    
    for (int i = 0; i < cycle->cycle_length; i++) {
        int vertex = cycle->cycle_path[i];
        
        if (i > 0) {
            printf(" -> ");
        }
        
        if (vertex >= 0 && vertex < graph->num_vertices) {
            if (graph->vertex_type[vertex] == VERTEX_TYPE_PROCESS) {
                printf("P%d[%d]", graph->vertex_id[vertex], vertex);
            } else if (graph->vertex_type[vertex] == VERTEX_TYPE_RESOURCE) {
                printf("R%d[%d]", graph->vertex_id[vertex], vertex);
            } else {
                printf("?[%d]", vertex);
            }
        } else {
            printf("?[%d]", vertex);
        }
    }
    
    if (cycle->num_processes > 0 && cycle->process_ids != NULL) {
        printf("\n  Processes: ");
        for (int i = 0; i < cycle->num_processes; i++) {
            if (i > 0) printf(", ");
            printf("PID=%d", cycle->process_ids[i]);
        }
    }
    
    if (cycle->num_resources > 0 && cycle->resource_ids != NULL) {
        printf("\n  Resources: ");
        for (int i = 0; i < cycle->num_resources; i++) {
            if (i > 0) printf(", ");
            printf("RID=%d", cycle->resource_ids[i]);
        }
    }
    
    printf("\n");
}

/*
 * print_all_cycles - Print all cycles found in graph
 * @cycle_list: Array of CycleInfo structures
 * @num_cycles: Number of cycles in array
 * @graph: ResourceGraph for vertex information
 * @return: None
 * Description: Prints all cycles with numbering and details.
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void print_all_cycles(const CycleInfo* cycle_list, int num_cycles,
                     const ResourceGraph* graph)
{
    if (cycle_list == NULL || graph == NULL) {
        printf("Invalid cycle list or graph\n");
        return;
    }
    
    if (num_cycles == 0) {
        printf("No cycles found\n");
        return;
    }
    
    printf("========================================\n");
    printf("Found %d cycle(s):\n", num_cycles);
    printf("========================================\n");
    
    for (int i = 0; i < num_cycles; i++) {
        printf("\nCycle #%d:\n", i + 1);
        print_cycle(&cycle_list[i], graph);
    }
    
    printf("========================================\n");
}

/*
 * free_cycle_info - Free memory allocated for a single CycleInfo
 * @cycle: CycleInfo structure to free
 * @return: None
 * Description: Frees all dynamically allocated arrays in CycleInfo.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer safely
 */
void free_cycle_info(CycleInfo* cycle)
{
    if (cycle == NULL) {
        return;
    }
    
    safe_free((void**)&cycle->cycle_path);
    safe_free((void**)&cycle->process_ids);
    safe_free((void**)&cycle->resource_ids);
    
    cycle->cycle_length = 0;
    cycle->num_processes = 0;
    cycle->num_resources = 0;
}

/*
 * free_cycle_list - Free array of CycleInfo structures
 * @cycle_list: Array of CycleInfo structures to free
 * @num_cycles: Number of cycles in array
 * @return: None
 * Description: Frees all cycles in array and the array itself.
 *              Time complexity: O(num_cycles)
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void free_cycle_list(CycleInfo* cycle_list, int num_cycles)
{
    if (cycle_list == NULL) {
        return;
    }
    
    if (num_cycles > 0) {
        for (int i = 0; i < num_cycles; i++) {
            free_cycle_info(&cycle_list[i]);
        }
    }
    
    free(cycle_list);
}

/*
 * validate_cycle - Validate that a cycle structure represents a real cycle
 * @cycle: CycleInfo structure to validate
 * @graph: ResourceGraph to validate against
 * @return: 1 if valid cycle, 0 otherwise
 * Description: Checks that cycle_path forms a valid cycle in the graph.
 *              Verifies edges exist and path is closed.
 *              Time complexity: O(cycle_length)
 * Error handling: Returns 0 for invalid or NULL cycles
 */
int validate_cycle(const CycleInfo* cycle, const ResourceGraph* graph)
{
    if (cycle == NULL || graph == NULL) {
        return 0;
    }
    
    if (cycle->cycle_path == NULL || cycle->cycle_length < 2) {
        return 0; /* Need at least 2 vertices for a cycle */
    }
    
    /* Check if path is closed (first == last) */
    if (cycle->cycle_path[0] != cycle->cycle_path[cycle->cycle_length - 1]) {
        return 0;
    }
    
    /* Check if all vertices are valid */
    for (int i = 0; i < cycle->cycle_length; i++) {
        int vertex = cycle->cycle_path[i];
        if (vertex < 0 || vertex >= graph->num_vertices) {
            return 0;
        }
    }
    
    /* Check if edges exist in graph */
    for (int i = 0; i < cycle->cycle_length - 1; i++) {
        int from = cycle->cycle_path[i];
        int to = cycle->cycle_path[i + 1];
        
        /* Check if edge exists in adjacency list */
        int edge_found = 0;
        GraphNode* current = graph->adjacency_list[from];
        while (current != NULL) {
            if (current->vertex_id == to) {
                edge_found = 1;
                break;
            }
            current = current->next;
        }
        
        if (!edge_found) {
            return 0; /* Edge doesn't exist */
        }
    }
    
    return 1; /* Valid cycle */
}

