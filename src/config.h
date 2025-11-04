#ifndef CONFIG_H
#define CONFIG_H

/* =============================================================================
 * CONFIG.H - Configuration Constants and Error Codes
 * =============================================================================
 * This header defines all system-wide constants, error codes, and configuration
 * macros used throughout the deadlock detection system.
 * =============================================================================
 */

#include <stddef.h>

/* =============================================================================
 * ERROR CODES
 * =============================================================================
 * Return codes: >= 0 for success, < 0 for errors
 */
#define SUCCESS 0
#define ERROR_FILE_NOT_FOUND -1
#define ERROR_PERMISSION_DENIED -2
#define ERROR_OUT_OF_MEMORY -3
#define ERROR_INVALID_ARGUMENT -4
#define ERROR_SYSTEM_CALL_FAILED -5
#define ERROR_GRAPH_CREATION_FAILED -6
#define ERROR_CYCLE_DETECTION_FAILED -7
#define ERROR_INVALID_PROCESS_ID -8
#define ERROR_BUFFER_OVERFLOW -9
#define ERROR_INVALID_FORMAT -10

/* =============================================================================
 * SYSTEM LIMITS
 * =============================================================================
 */
#define MAX_PROCESSES 10000
#define MAX_RESOURCES 5000
#define MAX_VERTICES (MAX_PROCESSES + MAX_RESOURCES)
#define MAX_EDGES 50000
#define MAX_PROCESS_NAME_LEN 256
#define MAX_PATH_LEN 4096
#define MAX_LINE_LEN 1024
#define MAX_FDS_PER_PROCESS 1024
#define MAX_RESOURCES_PER_PROCESS 256

/* =============================================================================
 * PROCESS STATES
 * =============================================================================
 * Process states as defined in /proc/[PID]/status
 */
#define PROCESS_STATE_RUNNING 'R'
#define PROCESS_STATE_SLEEPING 'S'
#define PROCESS_STATE_DISK_SLEEP 'D'
#define PROCESS_STATE_ZOMBIE 'Z'
#define PROCESS_STATE_TRACED 'T'
#define PROCESS_STATE_WAKE_KILL 'W'
#define PROCESS_STATE_DEAD 'X'

/* =============================================================================
 * GRAPH COLORS FOR DFS
 * =============================================================================
 * Used in cycle detection algorithm
 */
#define COLOR_WHITE 0  // Unvisited
#define COLOR_GRAY 1   // Currently being processed (in recursion stack)
#define COLOR_BLACK 2  // Finished processing

/* =============================================================================
 * VERTEX TYPES
 * =============================================================================
 */
#define VERTEX_TYPE_PROCESS 0
#define VERTEX_TYPE_RESOURCE 1

/* =============================================================================
 * RESOURCE TYPES
 * =============================================================================
 */
#define RESOURCE_TYPE_SINGLE_INSTANCE 0
#define RESOURCE_TYPE_MULTIPLE_INSTANCE 1

/* =============================================================================
 * OUTPUT FORMATS
 * =============================================================================
 */
#define OUTPUT_FORMAT_TEXT 0
#define OUTPUT_FORMAT_JSON 1
#define OUTPUT_FORMAT_VERBOSE 2

/* =============================================================================
 * DEBUG AND LOGGING
 * =============================================================================
 */
#ifndef DEBUG
#define DEBUG 0
#endif

/* =============================================================================
 * FILE PATHS
 * =============================================================================
 */
#define PROC_BASE_PATH "/proc"
#define PROC_STATUS_FILE "status"
#define PROC_FD_DIR "fd"
#define PROC_LOCKS_FILE "locks"
#define PROC_CMDLINE_FILE "cmdline"

/* =============================================================================
 * CACHE SETTINGS
 * =============================================================================
 */
#define CACHE_ENABLED 1
#define CACHE_TTL_SECONDS 5

/* =============================================================================
 * PERFORMANCE TUNING
 * =============================================================================
 */
#define DEFAULT_MONITORING_INTERVAL 5
#define MAX_MONITORING_INTERVAL 3600
#define MIN_MONITORING_INTERVAL 1

/* =============================================================================
 * VERSION INFORMATION
 * =============================================================================
 */
#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_STRING "1.0.0"

#endif /* CONFIG_H */

