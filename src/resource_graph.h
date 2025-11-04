#ifndef RESOURCE_GRAPH_H
#define RESOURCE_GRAPH_H

/* =============================================================================
 * RESOURCE_GRAPH.H - Resource Allocation Graph (RAG) Interface
 * =============================================================================
 * This header defines structures and functions for building and managing
 * Resource Allocation Graphs (RAG) used in deadlock detection.
 * =============================================================================
 */

#include "process_monitor.h"
#include "config.h"

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

/*
 * GraphNode - Node in adjacency list representation
 * Represents an edge in the graph
 */
typedef struct GraphNode {
    int vertex_id;                  /* ID of adjacent vertex */
    int edge_type;                  /* 0=request edge (P->R), 1=allocation edge (R->P) */
    struct GraphNode* next;         /* Next node in adjacency list */
} GraphNode;

/*
 * ResourceGraph - Resource Allocation Graph structure
 * Uses adjacency list representation for efficient sparse graph storage
 */
typedef struct {
    GraphNode** adjacency_list;     /* Array of adjacency lists, one per vertex */
    int* color;                     /* Color array for DFS: WHITE(0), GRAY(1), BLACK(2) */
    int* parent;                     /* Parent array for cycle path reconstruction */
    int num_vertices;                /* Current number of vertices in graph */
    int max_vertices;                /* Maximum capacity of vertices */
    int* vertex_type;                /* Array: 0=process, 1=resource for each vertex */
    int* vertex_id;                  /* Array: PID or RID for each vertex */
    int* vertex_instances;           /* Array: number of instances for resources, 0 for processes */
    int num_edges;                   /* Total number of edges in graph */
    int next_vertex_index;           /* Next available vertex index */
} ResourceGraph;

/* =============================================================================
 * FUNCTION PROTOTYPES
 * =============================================================================
 */

/*
 * create_graph - Create and initialize a new Resource Allocation Graph
 * @max_vertices: Maximum number of vertices the graph can hold
 * @return: Pointer to allocated ResourceGraph, or NULL on failure
 * Description: Allocates and initializes a new RAG with specified capacity.
 *              Uses adjacency list representation for efficient sparse graphs.
 *              Memory: O(V) where V is max_vertices.
 * Error handling: Returns NULL on allocation failure, logs error
 */
ResourceGraph* create_graph(int max_vertices);

/*
 * add_process_vertex - Add a process vertex to the graph
 * @graph: ResourceGraph to modify
 * @pid: Process ID to add
 * @return: Vertex index on success, -1 on failure
 * Description: Adds a process vertex to the graph if not already present.
 *              Returns the vertex index for this process.
 *              Time complexity: O(V) worst case to check for duplicates
 * Error handling: Returns -1 if graph is full or PID already exists
 */
int add_process_vertex(ResourceGraph* graph, int pid);

/*
 * add_resource_vertex - Add a resource vertex to the graph
 * @graph: ResourceGraph to modify
 * @rid: Resource ID to add
 * @instances: Number of instances for this resource (1 for single-instance)
 * @return: Vertex index on success, -1 on failure
 * Description: Adds a resource vertex with specified number of instances.
 *              Time complexity: O(V) worst case to check for duplicates
 * Error handling: Returns -1 if graph is full or RID already exists
 */
int add_resource_vertex(ResourceGraph* graph, int rid, int instances);

/*
 * add_request_edge - Add a request edge from process to resource
 * @graph: ResourceGraph to modify
 * @pid: Process ID requesting the resource
 * @rid: Resource ID being requested
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Adds edge P->R indicating process is waiting for resource.
 *              Creates vertices if they don't exist.
 *              Time complexity: O(V + E) worst case
 * Error handling: Returns error codes for invalid arguments or graph full
 */
int add_request_edge(ResourceGraph* graph, int pid, int rid);

/*
 * add_allocation_edge - Add an allocation edge from resource to process
 * @graph: ResourceGraph to modify
 * @rid: Resource ID being allocated
 * @pid: Process ID receiving the resource
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Adds edge R->P indicating resource is allocated to process.
 *              Creates vertices if they don't exist.
 *              Time complexity: O(V + E) worst case
 * Error handling: Returns error codes for invalid arguments or graph full
 */
int add_allocation_edge(ResourceGraph* graph, int rid, int pid);

/*
 * find_vertex_by_pid - Find vertex index for a given process ID
 * @graph: ResourceGraph to search
 * @pid: Process ID to find
 * @return: Vertex index if found, -1 if not found
 * Description: Searches for process vertex with given PID.
 *              Time complexity: O(V) linear search
 * Error handling: Returns -1 if not found or graph is NULL
 */
int find_vertex_by_pid(const ResourceGraph* graph, int pid);

/*
 * find_vertex_by_rid - Find vertex index for a given resource ID
 * @graph: ResourceGraph to search
 * @rid: Resource ID to find
 * @return: Vertex index if found, -1 if not found
 * Description: Searches for resource vertex with given RID.
 *              Time complexity: O(V) linear search
 * Error handling: Returns -1 if not found or graph is NULL
 */
int find_vertex_by_rid(const ResourceGraph* graph, int rid);

/*
 * get_vertex_id - Get the PID or RID for a vertex index
 * @graph: ResourceGraph to query
 * @vertex_index: Index of vertex
 * @return: PID or RID for vertex, -1 on error
 * Description: Returns the actual process or resource ID for a vertex.
 *              Time complexity: O(1)
 * Error handling: Returns -1 if vertex_index is invalid
 */
int get_vertex_id(const ResourceGraph* graph, int vertex_index);

/*
 * convert_to_wfg - Convert RAG to Wait-For Graph (WFG)
 * @rag: Source Resource Allocation Graph
 * @wfg_out: Output parameter for new WFG graph
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Converts RAG to WFG by removing resource nodes and creating
 *              direct edges between processes. Only includes processes that
 *              are part of a cycle.
 *              Time complexity: O(V + E)
 * Error handling: Returns error codes on allocation failure
 */
int convert_to_wfg(const ResourceGraph* rag, ResourceGraph** wfg_out);

/*
 * print_graph - Print graph structure for debugging
 * @graph: ResourceGraph to print
 * @return: None
 * Description: Prints adjacency list representation of graph to stdout.
 *              Shows all vertices and edges in readable format.
 *              Time complexity: O(V + E)
 * Error handling: Handles NULL graph safely
 */
void print_graph(const ResourceGraph* graph);

/*
 * get_graph_statistics - Get statistics about the graph
 * @graph: ResourceGraph to analyze
 * @num_processes: Output parameter for number of process vertices
 * @num_resources: Output parameter for number of resource vertices
 * @num_edges: Output parameter for total number of edges
 * @return: SUCCESS (0) on success
 * Description: Calculates and returns statistics about graph structure.
 *              Time complexity: O(V + E)
 * Error handling: Returns error codes for NULL parameters
 */
int get_graph_statistics(const ResourceGraph* graph, int* num_processes,
                        int* num_resources, int* num_edges);

/*
 * reset_graph_colors - Reset all vertex colors to WHITE
 * @graph: ResourceGraph to reset
 * @return: None
 * Description: Resets color array for new DFS traversal.
 *              Time complexity: O(V)
 * Error handling: Handles NULL graph safely
 */
void reset_graph_colors(ResourceGraph* graph);

/*
 * free_graph - Free all memory allocated for ResourceGraph
 * @graph: ResourceGraph to free
 * @return: None
 * Description: Frees adjacency lists, arrays, and graph structure itself.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer and partially initialized graphs safely
 */
void free_graph(ResourceGraph* graph);

#endif /* RESOURCE_GRAPH_H */

