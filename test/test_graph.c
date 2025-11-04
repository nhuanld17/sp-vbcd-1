/* =============================================================================
 * TEST_GRAPH.C - Resource Graph Unit Tests
 * =============================================================================
 * Tests for Resource Allocation Graph creation, manipulation, and cleanup.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/config.h"
#include "../src/utility.h"
#include "../src/resource_graph.h"

/* Test counters */
static int g_tests_passed = 0;
static int g_tests_failed = 0;

/* =============================================================================
 * TEST HELPERS
 * =============================================================================
 */

#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("  ✓ PASS: %s\n", message); \
            g_tests_passed++; \
        } else { \
            printf("  ✗ FAIL: %s\n", message); \
            g_tests_failed++; \
        } \
    } while (0)

/* =============================================================================
 * TEST FUNCTIONS
 * =============================================================================
 */

/*
 * test_graph_creation - Test graph creation and initialization
 */
static void test_graph_creation(void)
{
    printf("\n[TEST] Graph Creation\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(100);
    TEST_ASSERT(graph != NULL, "Graph creation should succeed");
    
    if (graph != NULL) {
        TEST_ASSERT(graph->num_vertices == 0, "Initial vertex count should be 0");
        TEST_ASSERT(graph->max_vertices == 100, "Max vertices should be 100");
        TEST_ASSERT(graph->num_edges == 0, "Initial edge count should be 0");
        TEST_ASSERT(graph->adjacency_list != NULL, "Adjacency list should be allocated");
        TEST_ASSERT(graph->color != NULL, "Color array should be allocated");
        TEST_ASSERT(graph->parent != NULL, "Parent array should be allocated");
        
        free_graph(graph);
        TEST_ASSERT(1, "Graph cleanup should succeed");
    }
}

/*
 * test_add_process_vertex - Test adding process vertices
 */
static void test_add_process_vertex(void)
{
    printf("\n[TEST] Add Process Vertices\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        int v1 = add_process_vertex(graph, 1001);
        TEST_ASSERT(v1 >= 0, "Add process PID 1001");
        TEST_ASSERT(graph->num_vertices == 1, "Vertex count should be 1");
        TEST_ASSERT(graph->vertex_type[v1] == VERTEX_TYPE_PROCESS, "Vertex type should be process");
        TEST_ASSERT(graph->vertex_id[v1] == 1001, "Vertex ID should be 1001");
        
        int v2 = add_process_vertex(graph, 1002);
        TEST_ASSERT(v2 >= 0 && v2 != v1, "Add process PID 1002");
        TEST_ASSERT(graph->num_vertices == 2, "Vertex count should be 2");
        
        /* Test duplicate */
        int v1_dup = add_process_vertex(graph, 1001);
        TEST_ASSERT(v1_dup == v1, "Duplicate PID should return same vertex");
        TEST_ASSERT(graph->num_vertices == 2, "Vertex count should still be 2");
        
        free_graph(graph);
    }
}

/*
 * test_add_resource_vertex - Test adding resource vertices
 */
static void test_add_resource_vertex(void)
{
    printf("\n[TEST] Add Resource Vertices\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        int r1 = add_resource_vertex(graph, 1, 1);
        TEST_ASSERT(r1 >= 0, "Add single-instance resource R1");
        TEST_ASSERT(graph->vertex_type[r1] == VERTEX_TYPE_RESOURCE, "Vertex type should be resource");
        TEST_ASSERT(graph->vertex_instances[r1] == 1, "Resource should have 1 instance");
        
        int r2 = add_resource_vertex(graph, 2, 3);
        TEST_ASSERT(r2 >= 0, "Add multi-instance resource R2");
        TEST_ASSERT(graph->vertex_instances[r2] == 3, "Resource should have 3 instances");
        
        free_graph(graph);
    }
}

/*
 * test_add_edges - Test adding request and allocation edges
 */
static void test_add_edges(void)
{
    printf("\n[TEST] Add Edges\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Add request edge: P1 -> R1 */
        int result1 = add_request_edge(graph, 1001, 1);
        TEST_ASSERT(result1 == SUCCESS, "Add request edge P1->R1");
        TEST_ASSERT(graph->num_edges >= 1, "Edge count should be >= 1");
        
        /* Add allocation edge: R1 -> P1 */
        int result2 = add_allocation_edge(graph, 1, 1001);
        TEST_ASSERT(result2 == SUCCESS, "Add allocation edge R1->P1");
        
        /* Verify vertices were created */
        int p1 = find_vertex_by_pid(graph, 1001);
        int r1 = find_vertex_by_rid(graph, 1);
        TEST_ASSERT(p1 >= 0, "Process vertex should exist");
        TEST_ASSERT(r1 >= 0, "Resource vertex should exist");
        
        /* Verify edges exist */
        GraphNode* p1_edges = graph->adjacency_list[p1];
        int found_request = 0;
        while (p1_edges != NULL) {
            if (p1_edges->vertex_id == r1 && p1_edges->edge_type == 0) {
                found_request = 1;
                break;
            }
            p1_edges = p1_edges->next;
        }
        TEST_ASSERT(found_request, "Request edge should exist in adjacency list");
        
        free_graph(graph);
    }
}

/*
 * test_large_graph - Test graph with many vertices
 */
static void test_large_graph(void)
{
    printf("\n[TEST] Large Graph\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(1000);
    TEST_ASSERT(graph != NULL, "Graph creation with 1000 capacity");
    
    if (graph != NULL) {
        /* Add many processes */
        int added = 0;
        for (int i = 1; i <= 500; i++) {
            int v = add_process_vertex(graph, 1000 + i);
            if (v >= 0) {
                added++;
            }
        }
        TEST_ASSERT(added == 500, "Should add 500 processes");
        
        /* Add many resources */
        int resources_added = 0;
        for (int i = 1; i <= 300; i++) {
            int v = add_resource_vertex(graph, i, 1);
            if (v >= 0) {
                resources_added++;
            }
        }
        TEST_ASSERT(resources_added == 300, "Should add 300 resources");
        
        /* Add edges */
        int edges_added = 0;
        for (int i = 1; i <= 100; i++) {
            if (add_request_edge(graph, 1000 + i, i) == SUCCESS) {
                edges_added++;
            }
        }
        TEST_ASSERT(edges_added == 100, "Should add 100 edges");
        
        /* Get statistics */
        int num_procs, num_resources, num_edges;
        int stat_result = get_graph_statistics(graph, &num_procs, &num_resources, &num_edges);
        TEST_ASSERT(stat_result == SUCCESS, "Get statistics should succeed");
        TEST_ASSERT(num_procs == 500, "Should have 500 processes");
        TEST_ASSERT(num_resources == 300, "Should have 300 resources");
        
        free_graph(graph);
        TEST_ASSERT(1, "Large graph cleanup should succeed");
    }
}

/*
 * test_graph_cleanup - Test graph cleanup and memory safety
 */
static void test_graph_cleanup(void)
{
    printf("\n[TEST] Graph Cleanup\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(100);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Add vertices and edges */
        add_process_vertex(graph, 1001);
        add_resource_vertex(graph, 1, 1);
        add_request_edge(graph, 1001, 1);
        add_allocation_edge(graph, 1, 1001);
        
        /* Cleanup should free all memory */
        free_graph(graph);
        graph = NULL;
        
        /* Test freeing NULL */
        free_graph(NULL);
        
        TEST_ASSERT(1, "Graph cleanup should be safe");
    }
}

/*
 * test_vertex_lookup - Test vertex lookup functions
 */
static void test_vertex_lookup(void)
{
    printf("\n[TEST] Vertex Lookup\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        int p1 = add_process_vertex(graph, 1001);
        int r1 = add_resource_vertex(graph, 1, 1);
        
        int found_p1 = find_vertex_by_pid(graph, 1001);
        TEST_ASSERT(found_p1 == p1, "Find process by PID should return correct index");
        
        int found_r1 = find_vertex_by_rid(graph, 1);
        TEST_ASSERT(found_r1 == r1, "Find resource by RID should return correct index");
        
        int not_found = find_vertex_by_pid(graph, 9999);
        TEST_ASSERT(not_found == -1, "Non-existent PID should return -1");
        
        int pid = get_vertex_id(graph, p1);
        TEST_ASSERT(pid == 1001, "Get vertex ID should return correct PID");
        
        free_graph(graph);
    }
}

/*
 * test_reset_colors - Test color reset functionality
 */
static void test_reset_colors(void)
{
    printf("\n[TEST] Reset Colors\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        int v1 = add_process_vertex(graph, 1001);
        
        /* Manually set colors */
        graph->color[v1] = COLOR_GRAY;
        graph->parent[v1] = 5;
        
        /* Reset */
        reset_graph_colors(graph);
        
        TEST_ASSERT(graph->color[v1] == COLOR_WHITE, "Color should be reset to WHITE");
        TEST_ASSERT(graph->parent[v1] == -1, "Parent should be reset to -1");
        
        free_graph(graph);
    }
}

/* =============================================================================
 * MAIN TEST RUNNER
 * =============================================================================
 */

int main(void)
{
    printf("========================================\n");
    printf("  RESOURCE GRAPH UNIT TESTS\n");
    printf("========================================\n");
    
    /* Run all tests */
    test_graph_creation();
    test_add_process_vertex();
    test_add_resource_vertex();
    test_add_edges();
    test_vertex_lookup();
    test_reset_colors();
    test_large_graph();
    test_graph_cleanup();
    
    /* Print summary */
    printf("\n========================================\n");
    printf("TEST SUMMARY\n");
    printf("========================================\n");
    printf("Passed: %d\n", g_tests_passed);
    printf("Failed: %d\n", g_tests_failed);
    printf("Total:  %d\n", g_tests_passed + g_tests_failed);
    printf("========================================\n");
    
    if (g_tests_failed == 0) {
        printf("ALL TESTS PASSED ✓\n");
        return 0;
    } else {
        printf("SOME TESTS FAILED ✗\n");
        return 1;
    }
}

