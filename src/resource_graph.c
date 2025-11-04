/* =============================================================================
 * RESOURCE_GRAPH.C - Resource Allocation Graph Implementation
 * =============================================================================
 * Implementation of Resource Allocation Graph (RAG) using adjacency list
 * representation for efficient sparse graph storage.
 * =============================================================================
 */

#include "resource_graph.h"
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
 * create_graph_node - Create a new graph node for adjacency list
 * @vertex_id: ID of adjacent vertex
 * @edge_type: Type of edge (0=request, 1=allocation)
 * @return: Pointer to new GraphNode, or NULL on failure
 */
static GraphNode* create_graph_node(int vertex_id, int edge_type)
{
    GraphNode* node = (GraphNode*)safe_malloc(sizeof(GraphNode));
    if (node == NULL) {
        return NULL;
    }
    
    node->vertex_id = vertex_id;
    node->edge_type = edge_type;
    node->next = NULL;
    
    return node;
}

/*
 * add_edge_to_list - Add edge to adjacency list
 * @head: Pointer to head of adjacency list (may be modified)
 * @vertex_id: ID of destination vertex
 * @edge_type: Type of edge (0=request, 1=allocation)
 * @return: SUCCESS (0) on success, negative on failure
 */
static int add_edge_to_list(GraphNode** head, int vertex_id, int edge_type)
{
    if (head == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Check if edge already exists */
    GraphNode* current = *head;
    while (current != NULL) {
        if (current->vertex_id == vertex_id && current->edge_type == edge_type) {
            /* Edge already exists, skip */
            return SUCCESS;
        }
        current = current->next;
    }
    
    /* Create new node */
    GraphNode* new_node = create_graph_node(vertex_id, edge_type);
    if (new_node == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Insert at head for O(1) insertion */
    new_node->next = *head;
    *head = new_node;
    
    return SUCCESS;
}

/* =============================================================================
 * MAIN FUNCTIONS
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
ResourceGraph* create_graph(int max_vertices)
{
    if (max_vertices <= 0) {
        error_log("create_graph: max_vertices must be positive");
        return NULL;
    }
    
    ResourceGraph* graph = (ResourceGraph*)safe_malloc(sizeof(ResourceGraph));
    if (graph == NULL) {
        return NULL;
    }
    
    /* Initialize graph structure */
    graph->num_vertices = 0;
    graph->max_vertices = max_vertices;
    graph->num_edges = 0;
    graph->next_vertex_index = 0;
    
    /* Allocate adjacency list array */
    graph->adjacency_list = (GraphNode**)safe_malloc(
        sizeof(GraphNode*) * max_vertices);
    if (graph->adjacency_list == NULL) {
        free(graph);
        return NULL;
    }
    
    /* Initialize all adjacency lists to NULL */
    for (int i = 0; i < max_vertices; i++) {
        graph->adjacency_list[i] = NULL;
    }
    
    /* Allocate color array */
    graph->color = (int*)safe_malloc(sizeof(int) * max_vertices);
    if (graph->color == NULL) {
        free(graph->adjacency_list);
        free(graph);
        return NULL;
    }
    
    /* Allocate parent array */
    graph->parent = (int*)safe_malloc(sizeof(int) * max_vertices);
    if (graph->parent == NULL) {
        free(graph->color);
        free(graph->adjacency_list);
        free(graph);
        return NULL;
    }
    
    /* Allocate vertex type array */
    graph->vertex_type = (int*)safe_malloc(sizeof(int) * max_vertices);
    if (graph->vertex_type == NULL) {
        free(graph->parent);
        free(graph->color);
        free(graph->adjacency_list);
        free(graph);
        return NULL;
    }
    
    /* Allocate vertex ID array */
    graph->vertex_id = (int*)safe_malloc(sizeof(int) * max_vertices);
    if (graph->vertex_id == NULL) {
        free(graph->vertex_type);
        free(graph->parent);
        free(graph->color);
        free(graph->adjacency_list);
        free(graph);
        return NULL;
    }
    
    /* Allocate vertex instances array */
    graph->vertex_instances = (int*)safe_malloc(sizeof(int) * max_vertices);
    if (graph->vertex_instances == NULL) {
        free(graph->vertex_id);
        free(graph->vertex_type);
        free(graph->parent);
        free(graph->color);
        free(graph->adjacency_list);
        free(graph);
        return NULL;
    }
    
    /* Initialize arrays */
    for (int i = 0; i < max_vertices; i++) {
        graph->color[i] = COLOR_WHITE;
        graph->parent[i] = -1;
        graph->vertex_type[i] = -1;
        graph->vertex_id[i] = -1;
        graph->vertex_instances[i] = 0;
    }
    
    return graph;
}

/*
 * find_vertex_by_pid - Find vertex index for a given process ID
 * @graph: ResourceGraph to search
 * @pid: Process ID to find
 * @return: Vertex index if found, -1 if not found
 * Description: Searches for process vertex with given PID.
 *              Time complexity: O(V) linear search
 * Error handling: Returns -1 if not found or graph is NULL
 */
int find_vertex_by_pid(const ResourceGraph* graph, int pid)
{
    if (graph == NULL) {
        return -1;
    }
    
    for (int i = 0; i < graph->num_vertices; i++) {
        if (graph->vertex_type[i] == VERTEX_TYPE_PROCESS &&
            graph->vertex_id[i] == pid) {
            return i;
        }
    }
    
    return -1;
}

/*
 * find_vertex_by_rid - Find vertex index for a given resource ID
 * @graph: ResourceGraph to search
 * @rid: Resource ID to find
 * @return: Vertex index if found, -1 if not found
 * Description: Searches for resource vertex with given RID.
 *              Time complexity: O(V) linear search
 * Error handling: Returns -1 if not found or graph is NULL
 */
int find_vertex_by_rid(const ResourceGraph* graph, int rid)
{
    if (graph == NULL) {
        return -1;
    }
    
    for (int i = 0; i < graph->num_vertices; i++) {
        if (graph->vertex_type[i] == VERTEX_TYPE_RESOURCE &&
            graph->vertex_id[i] == rid) {
            return i;
        }
    }
    
    return -1;
}

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
int add_process_vertex(ResourceGraph* graph, int pid)
{
    if (graph == NULL) {
        return -1;
    }
    
    if (pid <= 0) {
        error_log("add_process_vertex: invalid PID %d", pid);
        return -1;
    }
    
    /* Check if vertex already exists */
    int existing = find_vertex_by_pid(graph, pid);
    if (existing >= 0) {
        return existing;
    }
    
    /* Check if graph is full */
    if (graph->num_vertices >= graph->max_vertices) {
        error_log("add_process_vertex: graph is full (max_vertices=%d)",
                  graph->max_vertices);
        return -1;
    }
    
    /* Add new vertex */
    int vertex_index = graph->next_vertex_index++;
    graph->vertex_type[vertex_index] = VERTEX_TYPE_PROCESS;
    graph->vertex_id[vertex_index] = pid;
    graph->vertex_instances[vertex_index] = 0; /* Processes don't have instances */
    graph->color[vertex_index] = COLOR_WHITE;
    graph->parent[vertex_index] = -1;
    graph->adjacency_list[vertex_index] = NULL;
    
    graph->num_vertices++;
    
    return vertex_index;
}

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
int add_resource_vertex(ResourceGraph* graph, int rid, int instances)
{
    if (graph == NULL) {
        return -1;
    }
    
    if (rid < 0) {
        error_log("add_resource_vertex: invalid RID %d", rid);
        return -1;
    }
    
    if (instances <= 0) {
        instances = 1; /* Default to single instance */
    }
    
    /* Check if vertex already exists */
    int existing = find_vertex_by_rid(graph, rid);
    if (existing >= 0) {
        /* Update instances if different */
        if (graph->vertex_instances[existing] != instances) {
            graph->vertex_instances[existing] = instances;
        }
        return existing;
    }
    
    /* Check if graph is full */
    if (graph->num_vertices >= graph->max_vertices) {
        error_log("add_resource_vertex: graph is full (max_vertices=%d)",
                  graph->max_vertices);
        return -1;
    }
    
    /* Add new vertex */
    int vertex_index = graph->next_vertex_index++;
    graph->vertex_type[vertex_index] = VERTEX_TYPE_RESOURCE;
    graph->vertex_id[vertex_index] = rid;
    graph->vertex_instances[vertex_index] = instances;
    graph->color[vertex_index] = COLOR_WHITE;
    graph->parent[vertex_index] = -1;
    graph->adjacency_list[vertex_index] = NULL;
    
    graph->num_vertices++;
    
    return vertex_index;
}

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
int add_request_edge(ResourceGraph* graph, int pid, int rid)
{
    if (graph == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Ensure process vertex exists */
    int process_vertex = add_process_vertex(graph, pid);
    if (process_vertex < 0) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Ensure resource vertex exists (default to single instance) */
    int resource_vertex = add_resource_vertex(graph, rid, 1);
    if (resource_vertex < 0) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Add edge from process to resource (request edge) */
    int result = add_edge_to_list(&graph->adjacency_list[process_vertex],
                                  resource_vertex, 0); /* 0 = request edge */
    if (result == SUCCESS) {
        graph->num_edges++;
    }
    
    return result;
}

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
int add_allocation_edge(ResourceGraph* graph, int rid, int pid)
{
    if (graph == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Ensure resource vertex exists (default to single instance) */
    int resource_vertex = add_resource_vertex(graph, rid, 1);
    if (resource_vertex < 0) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Ensure process vertex exists */
    int process_vertex = add_process_vertex(graph, pid);
    if (process_vertex < 0) {
        return ERROR_GRAPH_CREATION_FAILED;
    }
    
    /* Add edge from resource to process (allocation edge) */
    int result = add_edge_to_list(&graph->adjacency_list[resource_vertex],
                                  process_vertex, 1); /* 1 = allocation edge */
    if (result == SUCCESS) {
        graph->num_edges++;
    }
    
    return result;
}

/*
 * get_vertex_id - Get the PID or RID for a vertex index
 * @graph: ResourceGraph to query
 * @vertex_index: Index of vertex
 * @return: PID or RID for vertex, -1 on error
 * Description: Returns the actual process or resource ID for a vertex.
 *              Time complexity: O(1)
 * Error handling: Returns -1 if vertex_index is invalid
 */
int get_vertex_id(const ResourceGraph* graph, int vertex_index)
{
    if (graph == NULL) {
        return -1;
    }
    
    if (vertex_index < 0 || vertex_index >= graph->num_vertices) {
        return -1;
    }
    
    return graph->vertex_id[vertex_index];
}

/*
 * print_graph - Print graph structure for debugging
 * @graph: ResourceGraph to print
 * @return: None
 * Description: Prints adjacency list representation of graph to stdout.
 *              Shows all vertices and edges in readable format.
 *              Time complexity: O(V + E)
 * Error handling: Handles NULL graph safely
 */
void print_graph(const ResourceGraph* graph)
{
    if (graph == NULL) {
        printf("Graph is NULL\n");
        return;
    }
    
    printf("========================================\n");
    printf("Resource Allocation Graph\n");
    printf("========================================\n");
    printf("Vertices: %d / %d\n", graph->num_vertices, graph->max_vertices);
    printf("Edges: %d\n", graph->num_edges);
    printf("----------------------------------------\n");
    
    for (int i = 0; i < graph->num_vertices; i++) {
        if (graph->vertex_type[i] == VERTEX_TYPE_PROCESS) {
            printf("Process[%d]: PID=%d", i, graph->vertex_id[i]);
        } else {
            printf("Resource[%d]: RID=%d (instances=%d)", 
                   i, graph->vertex_id[i], graph->vertex_instances[i]);
        }
        
        printf(" -> ");
        
        GraphNode* current = graph->adjacency_list[i];
        if (current == NULL) {
            printf("(no edges)");
        } else {
            int first = 1;
            while (current != NULL) {
                if (!first) {
                    printf(", ");
                }
                first = 0;
                
                if (graph->vertex_type[current->vertex_id] == VERTEX_TYPE_PROCESS) {
                    printf("P%d", graph->vertex_id[current->vertex_id]);
                } else {
                    printf("R%d", graph->vertex_id[current->vertex_id]);
                }
                
                if (current->edge_type == 0) {
                    printf("(req)");
                } else {
                    printf("(alloc)");
                }
                
                current = current->next;
            }
        }
        
        printf("\n");
    }
    
    printf("========================================\n");
}

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
                        int* num_resources, int* num_edges)
{
    if (graph == NULL || num_processes == NULL || 
        num_resources == NULL || num_edges == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *num_processes = 0;
    *num_resources = 0;
    *num_edges = graph->num_edges;
    
    for (int i = 0; i < graph->num_vertices; i++) {
        if (graph->vertex_type[i] == VERTEX_TYPE_PROCESS) {
            (*num_processes)++;
        } else if (graph->vertex_type[i] == VERTEX_TYPE_RESOURCE) {
            (*num_resources)++;
        }
    }
    
    return SUCCESS;
}

/*
 * reset_graph_colors - Reset all vertex colors to WHITE
 * @graph: ResourceGraph to reset
 * @return: None
 * Description: Resets color array for new DFS traversal.
 *              Time complexity: O(V)
 * Error handling: Handles NULL graph safely
 */
void reset_graph_colors(ResourceGraph* graph)
{
    if (graph == NULL) {
        return;
    }
    
    for (int i = 0; i < graph->num_vertices; i++) {
        graph->color[i] = COLOR_WHITE;
        graph->parent[i] = -1;
    }
}

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
int convert_to_wfg(const ResourceGraph* rag, ResourceGraph** wfg_out)
{
    if (rag == NULL || wfg_out == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Count process vertices */
    int process_count = 0;
    for (int i = 0; i < rag->num_vertices; i++) {
        if (rag->vertex_type[i] == VERTEX_TYPE_PROCESS) {
            process_count++;
        }
    }
    
    if (process_count == 0) {
        *wfg_out = NULL;
        return SUCCESS;
    }
    
    /* Create new WFG graph */
    ResourceGraph* wfg = create_graph(process_count);
    if (wfg == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Add all process vertices to WFG */
    for (int i = 0; i < rag->num_vertices; i++) {
        if (rag->vertex_type[i] == VERTEX_TYPE_PROCESS) {
            int pid = rag->vertex_id[i];
            add_process_vertex(wfg, pid);
        }
    }
    
    /* Convert edges: P1->R->P2 becomes P1->P2 in WFG */
    for (int i = 0; i < rag->num_vertices; i++) {
        if (rag->vertex_type[i] == VERTEX_TYPE_PROCESS) {
            GraphNode* current = rag->adjacency_list[i];
            while (current != NULL) {
                int resource_vertex = current->vertex_id;
                
                /* Follow resource's edges to find which process it's allocated to */
                if (rag->vertex_type[resource_vertex] == VERTEX_TYPE_RESOURCE) {
                    GraphNode* resource_edge = rag->adjacency_list[resource_vertex];
                    while (resource_edge != NULL) {
                        if (resource_edge->edge_type == 1) { /* Allocation edge */
                            int target_process = resource_edge->vertex_id;
                            
                            /* Add direct edge P1->P2 in WFG */
                            int p1_idx = find_vertex_by_pid(wfg, rag->vertex_id[i]);
                            int p2_idx = find_vertex_by_pid(wfg, rag->vertex_id[target_process]);
                            
                            if (p1_idx >= 0 && p2_idx >= 0) {
                                add_edge_to_list(&wfg->adjacency_list[p1_idx], 
                                                p2_idx, 0);
                                wfg->num_edges++;
                            }
                        }
                        resource_edge = resource_edge->next;
                    }
                }
                
                current = current->next;
            }
        }
    }
    
    *wfg_out = wfg;
    return SUCCESS;
}

/*
 * free_graph - Free all memory allocated for ResourceGraph
 * @graph: ResourceGraph to free
 * @return: None
 * Description: Frees adjacency lists, arrays, and graph structure itself.
 *              Safe to call with NULL pointer.
 * Error handling: Handles NULL pointer and partially initialized graphs safely
 */
void free_graph(ResourceGraph* graph)
{
    if (graph == NULL) {
        return;
    }
    
    /* Free all adjacency lists */
    if (graph->adjacency_list != NULL) {
        for (int i = 0; i < graph->max_vertices; i++) {
            GraphNode* current = graph->adjacency_list[i];
            while (current != NULL) {
                GraphNode* next = current->next;
                free(current);
                current = next;
            }
        }
        free(graph->adjacency_list);
    }
    
    /* Free arrays */
    safe_free((void**)&graph->color);
    safe_free((void**)&graph->parent);
    safe_free((void**)&graph->vertex_type);
    safe_free((void**)&graph->vertex_id);
    safe_free((void**)&graph->vertex_instances);
    
    /* Free graph structure */
    free(graph);
}

