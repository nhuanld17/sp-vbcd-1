/* =============================================================================
 * TEST_SYSTEM.C - System Integration Tests
 * =============================================================================
 * End-to-end integration tests for deadlock detection system.
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../src/config.h"
#include "../src/utility.h"
#include "../src/process_monitor.h"
#include "../src/resource_graph.h"
#include "../src/cycle_detection.h"
#include "../src/deadlock_detection.h"
#include "../src/output_handler.h"

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

/*
 * create_mock_process_data - Create mock ProcessResourceInfo for testing
 */
static ProcessResourceInfo* create_mock_process_data(int num_procs)
{
    ProcessResourceInfo* procs = (ProcessResourceInfo*)safe_malloc(
        sizeof(ProcessResourceInfo) * num_procs);
    if (procs == NULL) {
        return NULL;
    }
    
    for (int i = 0; i < num_procs; i++) {
        memset(&procs[i], 0, sizeof(ProcessResourceInfo));
        procs[i].pid = 1000 + i;
        procs[i].num_held = 0;
        procs[i].num_waiting = 0;
        procs[i].held_resources = NULL;
        procs[i].waiting_resources = NULL;
        procs[i].held_files = NULL;
        procs[i].waiting_files = NULL;
    }
    
    return procs;
}

/*
 * free_mock_process_data - Free mock process data
 */
static void free_mock_process_data(ProcessResourceInfo* procs, int num_procs)
{
    if (procs == NULL) {
        return;
    }
    
    for (int i = 0; i < num_procs; i++) {
        free_process_resource_info(&procs[i]);
    }
    free(procs);
}

/* =============================================================================
 * TEST FUNCTIONS
 * =============================================================================
 */

/*
 * test_build_rag_from_processes - Test RAG building from process data
 */
static void test_build_rag_from_processes(void)
{
    printf("\n[TEST] Build RAG from Processes\n");
    printf("----------------------------------------\n");
    
    ProcessResourceInfo* procs = create_mock_process_data(3);
    TEST_ASSERT(procs != NULL, "Create mock process data");
    
    if (procs != NULL) {
        /* Setup process 0: holds resource 1, waits for resource 2 */
        procs[0].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_held = 1;
        procs[0].held_resources[0] = 1;
        
        procs[0].waiting_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_waiting = 1;
        procs[0].waiting_resources[0] = 2;
        
        /* Setup process 1: holds resource 2, waits for resource 3 */
        procs[1].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[1].num_held = 1;
        procs[1].held_resources[0] = 2;
        
        procs[1].waiting_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[1].num_waiting = 1;
        procs[1].waiting_resources[0] = 3;
        
        /* Build RAG */
        ResourceGraph* graph = NULL;
        int result = build_rag_from_processes(procs, 3, &graph);
        
        TEST_ASSERT(result == SUCCESS, "Build RAG should succeed");
        TEST_ASSERT(graph != NULL, "Graph should be created");
        
        if (graph != NULL) {
            int num_procs, num_resources, num_edges;
            get_graph_statistics(graph, &num_procs, &num_resources, &num_edges);
            TEST_ASSERT(num_procs == 3, "Should have 3 process vertices");
            TEST_ASSERT(num_resources >= 2, "Should have at least 2 resource vertices");
            
            free_graph(graph);
        }
        
        free_mock_process_data(procs, 3);
    }
}

/*
 * test_deadlock_detection_no_deadlock - Test detection with no deadlock
 */
static void test_deadlock_detection_no_deadlock(void)
{
    printf("\n[TEST] Deadlock Detection - No Deadlock\n");
    printf("----------------------------------------\n");
    
    ProcessResourceInfo* procs = create_mock_process_data(2);
    TEST_ASSERT(procs != NULL, "Create mock process data");
    
    if (procs != NULL) {
        /* Linear chain: no cycle */
        procs[0].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_held = 1;
        procs[0].held_resources[0] = 1;
        
        procs[0].waiting_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_waiting = 1;
        procs[0].waiting_resources[0] = 2;
        
        procs[1].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[1].num_held = 1;
        procs[1].held_resources[0] = 2;
        
        DeadlockReport* report = create_deadlock_report();
        TEST_ASSERT(report != NULL, "Create deadlock report");
        
        if (report != NULL) {
            int result = detect_deadlock_in_system(procs, 2, report);
            TEST_ASSERT(result >= 0, "Detection should succeed");
            TEST_ASSERT(report->deadlock_detected == 0, "Should detect no deadlock");
            
            free_deadlock_report(report);
        }
        
        free_mock_process_data(procs, 2);
    }
}

/*
 * test_deadlock_detection_with_deadlock - Test detection with deadlock
 */
static void test_deadlock_detection_with_deadlock(void)
{
    printf("\n[TEST] Deadlock Detection - With Deadlock\n");
    printf("----------------------------------------\n");
    
    ProcessResourceInfo* procs = create_mock_process_data(2);
    TEST_ASSERT(procs != NULL, "Create mock process data");
    
    if (procs != NULL) {
        /* Create cycle: P1 holds R1, waits for R2; P2 holds R2, waits for R1 */
        procs[0].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_held = 1;
        procs[0].held_resources[0] = 1;
        
        procs[0].waiting_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[0].num_waiting = 1;
        procs[0].waiting_resources[0] = 2;
        
        procs[1].held_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[1].num_held = 1;
        procs[1].held_resources[0] = 2;
        
        procs[1].waiting_resources = (int*)safe_malloc(sizeof(int) * 1);
        procs[1].num_waiting = 1;
        procs[1].waiting_resources[0] = 1;
        
        DeadlockReport* report = create_deadlock_report();
        TEST_ASSERT(report != NULL, "Create deadlock report");
        
        if (report != NULL) {
            int result = detect_deadlock_in_system(procs, 2, report);
            TEST_ASSERT(result >= 0, "Detection should succeed");
            TEST_ASSERT(report->deadlock_detected == 1, "Should detect deadlock");
            TEST_ASSERT(report->num_deadlocked > 0, "Should have deadlocked processes");
            TEST_ASSERT(report->num_cycles > 0, "Should have cycles");
            
            free_deadlock_report(report);
        }
        
        free_mock_process_data(procs, 2);
    }
}

/*
 * test_output_formatting_text - Test TEXT output format
 */
static void test_output_formatting_text(void)
{
    printf("\n[TEST] Output Formatting - TEXT\n");
    printf("----------------------------------------\n");
    
    DeadlockReport* report = create_deadlock_report();
    TEST_ASSERT(report != NULL, "Create report");
    
    if (report != NULL) {
        report->deadlock_detected = 1;
        report->num_deadlocked = 2;
        report->deadlocked_pids = (int*)safe_malloc(sizeof(int) * 2);
        report->deadlocked_pids[0] = 1001;
        report->deadlocked_pids[1] = 1002;
        
        char buffer[4096];
        int len = format_as_text(report, buffer, sizeof(buffer));
        TEST_ASSERT(len > 0, "Format as text should succeed");
        TEST_ASSERT(strstr(buffer, "DEADLOCK") != NULL || 
                   strstr(buffer, "Deadlock") != NULL, 
                   "Output should contain deadlock information");
        
        free_deadlock_report(report);
    }
}

/*
 * test_output_formatting_json - Test JSON output format
 */
static void test_output_formatting_json(void)
{
    printf("\n[TEST] Output Formatting - JSON\n");
    printf("----------------------------------------\n");
    
    DeadlockReport* report = create_deadlock_report();
    TEST_ASSERT(report != NULL, "Create report");
    
    if (report != NULL) {
        report->deadlock_detected = 1;
        report->num_deadlocked = 1;
        report->deadlocked_pids = (int*)safe_malloc(sizeof(int) * 1);
        report->deadlocked_pids[0] = 1001;
        
        char buffer[4096];
        int len = format_as_json(report, buffer, sizeof(buffer));
        TEST_ASSERT(len > 0, "Format as JSON should succeed");
        TEST_ASSERT(strstr(buffer, "{") != NULL, "JSON should start with {");
        TEST_ASSERT(strstr(buffer, "deadlock_detected") != NULL, 
                   "JSON should contain deadlock_detected field");
        
        free_deadlock_report(report);
    }
}

/*
 * test_output_formatting_verbose - Test VERBOSE output format
 */
static void test_output_formatting_verbose(void)
{
    printf("\n[TEST] Output Formatting - VERBOSE\n");
    printf("----------------------------------------\n");
    
    DeadlockReport* report = create_deadlock_report();
    TEST_ASSERT(report != NULL, "Create report");
    
    if (report != NULL) {
        report->deadlock_detected = 1;
        report->num_deadlocked = 1;
        report->deadlocked_pids = (int*)safe_malloc(sizeof(int) * 1);
        report->deadlocked_pids[0] = 1001;
        
        char buffer[8192];
        int len = format_as_verbose(report, buffer, sizeof(buffer));
        TEST_ASSERT(len > 0, "Format as verbose should succeed");
        TEST_ASSERT(strstr(buffer, "DETAILED") != NULL || 
                   strstr(buffer, "Detailed") != NULL,
                   "Verbose output should contain detailed information");
        
        free_deadlock_report(report);
    }
}

/*
 * test_format_parsing - Test format string parsing
 */
static void test_format_parsing(void)
{
    printf("\n[TEST] Format Parsing\n");
    printf("----------------------------------------\n");
    
    OutputFormat fmt;
    
    int result1 = parse_output_format("text", &fmt);
    TEST_ASSERT(result1 == SUCCESS && fmt == FORMAT_TEXT, "Parse 'text' format");
    
    int result2 = parse_output_format("json", &fmt);
    TEST_ASSERT(result2 == SUCCESS && fmt == FORMAT_JSON, "Parse 'json' format");
    
    int result3 = parse_output_format("verbose", &fmt);
    TEST_ASSERT(result3 == SUCCESS && fmt == FORMAT_VERBOSE, "Parse 'verbose' format");
    
    int result4 = parse_output_format("invalid", &fmt);
    TEST_ASSERT(result4 == ERROR_INVALID_FORMAT, "Invalid format should return error");
}

/*
 * test_report_creation_cleanup - Test report creation and cleanup
 */
static void test_report_creation_cleanup(void)
{
    printf("\n[TEST] Report Creation and Cleanup\n");
    printf("----------------------------------------\n");
    
    DeadlockReport* report = create_deadlock_report();
    TEST_ASSERT(report != NULL, "Create report should succeed");
    
    if (report != NULL) {
        TEST_ASSERT(report->deadlock_detected == 0, "Report should be initialized");
        TEST_ASSERT(report->num_deadlocked == 0, "Initial deadlocked count should be 0");
        TEST_ASSERT(report->timestamp > 0, "Timestamp should be set");
        
        free_deadlock_report(report);
        report = NULL;
        
        free_deadlock_report(NULL); /* Test NULL safety */
        
        TEST_ASSERT(1, "Report cleanup should be safe");
    }
}

/* =============================================================================
 * MAIN TEST RUNNER
 * =============================================================================
 */

int main(void)
{
    printf("========================================\n");
    printf("  SYSTEM INTEGRATION TESTS\n");
    printf("========================================\n");
    
    /* Run all tests */
    test_build_rag_from_processes();
    test_deadlock_detection_no_deadlock();
    test_deadlock_detection_with_deadlock();
    test_output_formatting_text();
    test_output_formatting_json();
    test_output_formatting_verbose();
    test_format_parsing();
    test_report_creation_cleanup();
    
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

