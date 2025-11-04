/* =============================================================================
 * TEST_CYCLE.C - Cycle Detection Unit Tests
 * =============================================================================
 * Tests for DFS-based cycle detection algorithm.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/config.h"
#include "../src/utility.h"
#include "../src/resource_graph.h"
#include "../src/cycle_detection.h"

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
 * test_no_cycles - Test graph with no cycles
 */
static void test_no_cycles(void)
{
    printf("\n[TEST] No Cycles\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Create linear chain: P1 -> R1 -> P2 -> R2 (no cycle) */
        add_process_vertex(graph, 1001);
        add_resource_vertex(graph, 1, 1);
        add_process_vertex(graph, 1002);
        add_resource_vertex(graph, 2, 1);
        
        add_request_edge(graph, 1001, 1);
        add_allocation_edge(graph, 1, 1002);
        add_request_edge(graph, 1002, 2);
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles == 0, "Should find no cycles");
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_simple_cycle - Test simple 2-node cycle
 */
static void test_simple_cycle(void)
{
    printf("\n[TEST] Simple Cycle (2 nodes)\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Create cycle: P1 -> R1 -> P1 */
        add_request_edge(graph, 1001, 1);      /* P1 -> R1 */
        add_allocation_edge(graph, 1, 1001);  /* R1 -> P1 */
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles > 0, "Should find at least one cycle");
        
        if (num_cycles > 0 && cycles != NULL) {
            TEST_ASSERT(cycles[0].cycle_length >= 2, "Cycle should have at least 2 vertices");
            TEST_ASSERT(validate_cycle(&cycles[0], graph), "Cycle should be valid");
        }
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_complex_cycle - Test complex cycle with multiple processes and resources
 */
static void test_complex_cycle(void)
{
    printf("\n[TEST] Complex Cycle\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Create cycle: P1 -> R1 -> P2 -> R2 -> P1 */
        add_request_edge(graph, 1001, 1);      /* P1 -> R1 */
        add_allocation_edge(graph, 1, 1002);   /* R1 -> P2 */
        add_request_edge(graph, 1002, 2);      /* P2 -> R2 */
        add_allocation_edge(graph, 2, 1001);   /* R2 -> P1 */
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles > 0, "Should find at least one cycle");
        
        if (num_cycles > 0 && cycles != NULL) {
            TEST_ASSERT(cycles[0].cycle_length >= 4, "Cycle should have at least 4 vertices");
            TEST_ASSERT(validate_cycle(&cycles[0], graph), "Cycle should be valid");
        }
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_multiple_cycles - Test graph with multiple disjoint cycles
 */
static void test_multiple_cycles(void)
{
    printf("\n[TEST] Multiple Cycles\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(100);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Cycle 1: P1 -> R1 -> P1 */
        add_request_edge(graph, 1001, 1);
        add_allocation_edge(graph, 1, 1001);
        
        /* Cycle 2: P2 -> R2 -> P3 -> R3 -> P2 */
        add_request_edge(graph, 1002, 2);
        add_allocation_edge(graph, 2, 1003);
        add_request_edge(graph, 1003, 3);
        add_allocation_edge(graph, 3, 1002);
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles >= 2, "Should find at least 2 cycles");
        
        if (num_cycles >= 2 && cycles != NULL) {
            for (int i = 0; i < num_cycles; i++) {
                TEST_ASSERT(validate_cycle(&cycles[i], graph), 
                           "All cycles should be valid");
            }
        }
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_cycle_path_extraction - Test cycle path extraction
 */
static void test_cycle_path_extraction(void)
{
    printf("\n[TEST] Cycle Path Extraction\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Create cycle: P1 -> R1 -> P2 -> R2 -> P1 */
        add_request_edge(graph, 1001, 1);
        add_allocation_edge(graph, 1, 1002);
        add_request_edge(graph, 1002, 2);
        add_allocation_edge(graph, 2, 1001);
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles > 0, "Should find cycle");
        
        if (num_cycles > 0 && cycles != NULL) {
            CycleInfo* cycle = &cycles[0];
            TEST_ASSERT(cycle->cycle_path != NULL, "Cycle path should not be NULL");
            TEST_ASSERT(cycle->cycle_length > 0, "Cycle length should be > 0");
            
            /* Check path is closed */
            if (cycle->cycle_length > 0) {
                TEST_ASSERT(cycle->cycle_path[0] == 
                           cycle->cycle_path[cycle->cycle_length - 1],
                           "Cycle path should be closed");
            }
        }
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_has_cycle_function - Test has_cycle() wrapper function
 */
static void test_has_cycle_function(void)
{
    printf("\n[TEST] has_cycle() Function\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        /* Graph with no cycles */
        add_request_edge(graph, 1001, 1);
        add_allocation_edge(graph, 1, 1002);
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = has_cycle(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result >= 0, "has_cycle should not fail");
        TEST_ASSERT(result == 0 || num_cycles == 0, "Should detect no cycles");
        
        free_cycle_list(cycles, num_cycles);
        
        /* Add cycle */
        add_request_edge(graph, 1002, 2);
        add_allocation_edge(graph, 2, 1001);
        
        reset_graph_colors(graph);
        cycles = NULL;
        num_cycles = 0;
        result = has_cycle(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result > 0 || num_cycles > 0, "Should detect cycles");
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_empty_graph - Test cycle detection on empty graph
 */
static void test_empty_graph(void)
{
    printf("\n[TEST] Empty Graph\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles == 0, "Empty graph should have no cycles");
        
        free_cycle_list(cycles, num_cycles);
        free_graph(graph);
    }
}

/*
 * test_single_vertex - Test graph with single vertex
 */
static void test_single_vertex(void)
{
    printf("\n[TEST] Single Vertex\n");
    printf("----------------------------------------\n");
    
    ResourceGraph* graph = create_graph(50);
    TEST_ASSERT(graph != NULL, "Graph creation");
    
    if (graph != NULL) {
        add_process_vertex(graph, 1001);
        
        CycleInfo* cycles = NULL;
        int num_cycles = 0;
        int result = find_all_cycles(graph, &cycles, &num_cycles);
        
        TEST_ASSERT(result == SUCCESS, "Cycle detection should succeed");
        TEST_ASSERT(num_cycles == 0, "Single vertex should have no cycles");
        
        free_cycle_list(cycles, num_cycles);
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
    printf("  CYCLE DETECTION UNIT TESTS\n");
    printf("========================================\n");
    
    /* Run all tests */
    test_no_cycles();
    test_simple_cycle();
    test_complex_cycle();
    test_multiple_cycles();
    test_cycle_path_extraction();
    test_has_cycle_function();
    test_empty_graph();
    test_single_vertex();
    
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

