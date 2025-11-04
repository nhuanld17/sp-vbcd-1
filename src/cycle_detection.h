#ifndef CYCLE_DETECTION_H
#define CYCLE_DETECTION_H

/* =============================================================================
 * CYCLE_DETECTION.H - Cycle Detection Algorithm Interface
 * =============================================================================
 * This header defines structures and functions for detecting cycles in
 * Resource Allocation Graphs using Depth-First Search (DFS) algorithm.
 * =============================================================================
 */

#include "resource_graph.h"
#include "config.h"

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

/*
 * CycleInfo - Information about a detected cycle
 * Stores the complete path of vertices forming a cycle
 */
typedef struct {
    int* cycle_path;                /* Array of vertex indices forming the cycle */
    int cycle_length;               /* Number of vertices in cycle */
    int cycle_start_vertex;         /* Starting vertex index of cycle */
    int cycle_end_vertex;           /* Ending vertex index of cycle (back edge target) */
    int* process_ids;                /* Array of PIDs in cycle (if applicable) */
    int* resource_ids;              /* Array of RIDs in cycle (if applicable) */
    int num_processes;              /* Number of processes in cycle */
    int num_resources;              /* Number of resources in cycle */
} CycleInfo;

/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

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
int has_cycle(ResourceGraph* graph, CycleInfo** cycle_list, int* num_cycles);

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
int find_all_cycles(ResourceGraph* graph, CycleInfo** cycle_list, int* num_cycles);

/*
 * dfs_visit - Recursive DFS visit for cycle detection
 * @graph: ResourceGraph being traversed
 * @vertex: Current vertex being visited
 * @color: Color array for DFS marking
 * @parent: Parent array for path reconstruction
 * @cycle_info: Output structure for detected cycle (if found)
 * @found: Flag indicating if cycle was found
 * @return: SUCCESS (0) on success, negative on error
 * Description: Recursive DFS function that marks vertices and detects back edges.
 *              When a back edge (edge to GRAY vertex) is found, a cycle is detected.
 *              Time complexity: O(1) per call, O(V+E) total
 * Error handling: Returns error codes for invalid parameters
 */
int dfs_visit(ResourceGraph* graph, int vertex, int* color, int* parent,
              CycleInfo* cycle_info, int* found);

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
                       const ResourceGraph* graph, CycleInfo* cycle_info);

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
                     const int* color);

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
void print_cycle(const CycleInfo* cycle, const ResourceGraph* graph);

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
                     const ResourceGraph* graph);

/*
 * free_cycle_info - Free memory allocated for a single CycleInfo
 * @cycle: CycleInfo structure to free
 * @return: None
 * Description: Frees all dynamically allocated arrays in CycleInfo.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer safely
 */
void free_cycle_info(CycleInfo* cycle);

/*
 * free_cycle_list - Free array of CycleInfo structures
 * @cycle_list: Array of CycleInfo structures to free
 * @num_cycles: Number of cycles in array
 * @return: None
 * Description: Frees all cycles in array and the array itself.
 *              Time complexity: O(num_cycles)
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void free_cycle_list(CycleInfo* cycle_list, int num_cycles);

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
int validate_cycle(const CycleInfo* cycle, const ResourceGraph* graph);

#endif /* CYCLE_DETECTION_H */

