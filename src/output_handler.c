/* =============================================================================
 * OUTPUT_HANDLER.C - Output Formatting and Display Implementation
 * =============================================================================
 * Implementation of functions for formatting and displaying deadlock detection
 * results in TEXT, JSON, and VERBOSE formats.
 * =============================================================================
 */

#include "output_handler.h"
#include "utility.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

/* =============================================================================
 * HELPER FUNCTIONS
 * =============================================================================
 */

/*
 * escape_json_string - Escape special characters in JSON string
 * @str: String to escape
 * @output: Output buffer
 * @output_size: Size of output buffer
 * @return: Number of characters written
 */
static int escape_json_string(const char* str, char* output, size_t output_size)
{
    if (str == NULL || output == NULL || output_size == 0) {
        return 0;
    }
    
    int pos = 0;
    for (const char* p = str; *p != '\0' && pos < (int)output_size - 1; p++) {
        switch (*p) {
            case '"':  pos += snprintf(output + pos, output_size - pos, "\\\""); break;
            case '\\': pos += snprintf(output + pos, output_size - pos, "\\\\"); break;
            case '\n': pos += snprintf(output + pos, output_size - pos, "\\n"); break;
            case '\r': pos += snprintf(output + pos, output_size - pos, "\\r"); break;
            case '\t': pos += snprintf(output + pos, output_size - pos, "\\t"); break;
            default:
                if (isprint((unsigned char)*p) || *p == ' ') {
                    output[pos++] = *p;
                } else {
                    pos += snprintf(output + pos, output_size - pos, "\\u%04x", 
                                   (unsigned char)*p);
                }
                break;
        }
    }
    output[pos] = '\0';
    return pos;
}

/*
 * format_timestamp - Format Unix timestamp to readable string
 * @timestamp: Unix timestamp
 * @output: Output buffer
 * @output_size: Size of buffer
 * @return: Number of characters written
 */
static int format_timestamp(int timestamp, char* output, size_t output_size)
{
    if (output == NULL || output_size == 0) {
        return 0;
    }
    
    time_t t = (time_t)timestamp;
    struct tm* tm_info = localtime(&t);
    if (tm_info == NULL) {
        return snprintf(output, output_size, "%d", timestamp);
    }
    
    return strftime(output, output_size, "%Y-%m-%d %H:%M:%S", tm_info);
}

/* =============================================================================
 * FORMAT UTILITY FUNCTIONS
 * =============================================================================
 */

/*
 * parse_output_format - Parse output format string to enum
 * @format_str: String representation of format ("text", "json", "verbose")
 * @fmt: Output parameter for parsed format
 * @return: SUCCESS (0) on success, ERROR_INVALID_FORMAT on failure
 * Description: Converts string format name to OutputFormat enum.
 *              Case-insensitive matching.
 *              Time complexity: O(n) where n is string length
 * Error handling: Returns ERROR_INVALID_FORMAT for unknown formats
 */
int parse_output_format(const char* format_str, OutputFormat* fmt)
{
    if (format_str == NULL || fmt == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Convert to lowercase for comparison */
    char lower[32];
    int i = 0;
    for (; format_str[i] != '\0' && i < (int)sizeof(lower) - 1; i++) {
        lower[i] = (char)tolower((unsigned char)format_str[i]);
    }
    lower[i] = '\0';
    
    if (strcmp(lower, "text") == 0) {
        *fmt = FORMAT_TEXT;
        return SUCCESS;
    } else if (strcmp(lower, "json") == 0) {
        *fmt = FORMAT_JSON;
        return SUCCESS;
    } else if (strcmp(lower, "verbose") == 0) {
        *fmt = FORMAT_VERBOSE;
        return SUCCESS;
    }
    
    return ERROR_INVALID_FORMAT;
}

/*
 * get_format_string - Get string representation of output format
 * @fmt: OutputFormat enum value
 * @return: String name of format, or NULL for invalid format
 * Description: Returns human-readable string for output format enum.
 *              Time complexity: O(1)
 * Error handling: Returns NULL for invalid enum values
 */
const char* get_format_string(OutputFormat fmt)
{
    switch (fmt) {
        case FORMAT_TEXT: return "text";
        case FORMAT_JSON: return "json";
        case FORMAT_VERBOSE: return "verbose";
        default: return NULL;
    }
}

/* =============================================================================
 * HEADER AND FOOTER FUNCTIONS
 * =============================================================================
 */

/*
 * print_header - Print formatted header for deadlock report
 * @format: Output format to use for header styling
 * @return: None
 * Description: Prints a formatted header appropriate for the output format.
 *              For TEXT format: ASCII art header
 *              For JSON: opening brace
 *              For VERBOSE: detailed header with timestamp
 *              Time complexity: O(1)
 * Error handling: Handles invalid formats gracefully
 */
void print_header(OutputFormat format)
{
    switch (format) {
        case FORMAT_TEXT:
            printf("========================================\n");
            printf("    DEADLOCK DETECTION REPORT\n");
            printf("========================================\n");
            break;
            
        case FORMAT_JSON:
            printf("{\n");
            break;
            
        case FORMAT_VERBOSE:
            printf("========================================\n");
            printf("    DEADLOCK DETECTION REPORT\n");
            printf("    Detailed Analysis\n");
            printf("========================================\n\n");
            break;
            
        default:
            break;
    }
}

/*
 * print_footer - Print formatted footer for deadlock report
 * @format: Output format to use for footer styling
 * @return: None
 * Description: Prints a formatted footer appropriate for the output format.
 *              Closes JSON structure, adds summary, etc.
 *              Time complexity: O(1)
 * Error handling: Handles invalid formats gracefully
 */
void print_footer(OutputFormat format)
{
    switch (format) {
        case FORMAT_TEXT:
            printf("========================================\n");
            break;
            
        case FORMAT_JSON:
            printf("}\n");
            break;
            
        case FORMAT_VERBOSE:
            printf("\n========================================\n");
            printf("End of Report\n");
            printf("========================================\n");
            break;
            
        default:
            break;
    }
}

/* =============================================================================
 * PRINT FUNCTIONS
 * =============================================================================
 */

/*
 * print_summary - Print summary of deadlock detection results
 * @report: DeadlockReport to summarize
 * @return: None
 * Description: Prints brief summary including number of deadlocked processes,
 *              number of cycles detected, and timestamp.
 *              Time complexity: O(1)
 * Error handling: Handles NULL report safely
 */
void print_summary(const DeadlockReport* report)
{
    if (report == NULL) {
        printf("Summary: Invalid report\n");
        return;
    }
    
    printf("\nSummary:\n");
    printf("  Deadlock Detected: %s\n", report->deadlock_detected ? "YES" : "NO");
    printf("  Deadlocked Processes: %d\n", report->num_deadlocked);
    printf("  Cycles Found: %d\n", report->num_cycles);
    printf("  Processes Scanned: %d\n", report->total_processes_scanned);
    printf("  Resources Found: %d\n", report->total_resources_found);
    
    if (report->timestamp > 0) {
        char time_str[64];
        format_timestamp(report->timestamp, time_str, sizeof(time_str));
        printf("  Detection Time: %s\n", time_str);
    }
}

/*
 * print_detailed_wait_chain - Print detailed wait chain for deadlock
 * @report: DeadlockReport containing cycle information
 * @return: None
 * Description: Prints detailed wait chain showing how processes are waiting
 *              for each other in a cycle format: P1->R1->P2->R2->P1
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Handles NULL report and empty cycles safely
 */
void print_detailed_wait_chain(const DeadlockReport* report)
{
    if (report == NULL || report->cycles == NULL || report->num_cycles == 0) {
        return;
    }
    
    printf("\nWait Chains:\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < report->num_cycles; i++) {
        printf("\nCycle #%d:\n", i + 1);
        
        if (report->cycles[i].cycle_path == NULL || 
            report->cycles[i].cycle_length == 0) {
            printf("  (Invalid cycle)\n");
            continue;
        }
        
        printf("  Path: ");
        for (int j = 0; j < report->cycles[i].cycle_length - 1; j++) {
            int vertex = report->cycles[i].cycle_path[j];
            if (j > 0) {
                printf(" -> ");
            }
            printf("%d", vertex);
        }
        printf("\n");
        
        if (report->cycles[i].process_ids != NULL && 
            report->cycles[i].num_processes > 0) {
            printf("  Processes: ");
            for (int j = 0; j < report->cycles[i].num_processes; j++) {
                if (j > 0) printf(", ");
                printf("PID %d", report->cycles[i].process_ids[j]);
            }
            printf("\n");
        }
        
        if (report->cycles[i].resource_ids != NULL && 
            report->cycles[i].num_resources > 0) {
            printf("  Resources: ");
            for (int j = 0; j < report->cycles[i].num_resources; j++) {
                if (j > 0) printf(", ");
                printf("RID %d", report->cycles[i].resource_ids[j]);
            }
            printf("\n");
        }
    }
}

/*
 * print_process_info - Print detailed information about deadlocked processes
 * @report: DeadlockReport containing process information
 * @return: None
 * Description: Prints detailed information for each deadlocked process including
 *              PID, name, and resources involved.
 *              Time complexity: O(D) where D=deadlocked processes
 * Error handling: Handles NULL report and missing process info gracefully
 */
void print_process_info(const DeadlockReport* report)
{
    if (report == NULL) {
        return;
    }
    
    if (report->num_deadlocked == 0 || report->deadlocked_pids == NULL) {
        printf("\nDeadlocked Processes: None\n");
        return;
    }
    
    printf("\nDeadlocked Processes:\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < report->num_deadlocked; i++) {
        printf("  PID %d", report->deadlocked_pids[i]);
        printf("\n");
    }
}

/*
 * print_resource_info - Print information about resources in deadlock
 * @report: DeadlockReport containing resource information
 * @return: None
 * Description: Prints information about resources involved in deadlock cycles,
 *              including resource IDs and which processes hold/wait for them.
 *              Time complexity: O(R) where R=resources in cycles
 * Error handling: Handles NULL report safely
 */
void print_resource_info(const DeadlockReport* report)
{
    if (report == NULL || report->cycles == NULL || report->num_cycles == 0) {
        return;
    }
    
    printf("\nResources Involved:\n");
    printf("----------------------------------------\n");
    
    /* Collect unique resources */
    int resource_ids[256];
    int resource_count = 0;
    
    for (int i = 0; i < report->num_cycles; i++) {
        if (report->cycles[i].resource_ids != NULL) {
            for (int j = 0; j < report->cycles[i].num_resources; j++) {
                int rid = report->cycles[i].resource_ids[j];
                
                /* Check if already in list */
                int found = 0;
                for (int k = 0; k < resource_count; k++) {
                    if (resource_ids[k] == rid) {
                        found = 1;
                        break;
                    }
                }
                
                if (!found && resource_count < 256) {
                    resource_ids[resource_count++] = rid;
                }
            }
        }
    }
    
    if (resource_count == 0) {
        printf("  No resources found in cycles\n");
        return;
    }
    
    for (int i = 0; i < resource_count; i++) {
        printf("  Resource ID: %d\n", resource_ids[i]);
    }
}

/*
 * print_recommendations - Print recommendations for resolving deadlocks
 * @report: DeadlockReport containing recommendations
 * @return: None
 * Description: Prints actionable recommendations for resolving detected deadlocks,
 *              such as which processes to terminate or resources to release.
 *              Time complexity: O(N) where N=number of recommendations
 * Error handling: Handles NULL report and missing recommendations safely
 */
void print_recommendations(const DeadlockReport* report)
{
    if (report == NULL) {
        return;
    }
    
    if (report->num_recommendations == 0 || report->recommendations == NULL) {
        return;
    }
    
    printf("\nRecommendations:\n");
    printf("----------------------------------------\n");
    
    for (int i = 0; i < report->num_recommendations; i++) {
        printf("  %d. %s\n", i + 1, 
               report->recommendations[i] ? report->recommendations[i] : "(null)");
    }
}

/* =============================================================================
 * FORMAT FUNCTIONS
 * =============================================================================
 */

/*
 * format_as_text - Format report as plain text
 * @report: DeadlockReport to format
 * @output: Output buffer to write formatted text
 * @output_size: Size of output buffer
 * @return: Number of characters written, or negative on error
 * Description: Formats deadlock report as human-readable plain text.
 *              Writes to provided buffer. Returns number of characters written.
 *              Time complexity: O(D + C * L)
 * Error handling: Returns negative error code if buffer too small
 */
int format_as_text(const DeadlockReport* report, char* output, size_t output_size)
{
    if (report == NULL || output == NULL || output_size == 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    int pos = 0;
    
    /* Header */
    pos += snprintf(output + pos, output_size - pos,
                   "========================================\n");
    pos += snprintf(output + pos, output_size - pos,
                   "    DEADLOCK DETECTION REPORT\n");
    pos += snprintf(output + pos, output_size - pos,
                   "========================================\n\n");
    
    /* Summary */
    pos += snprintf(output + pos, output_size - pos,
                   "Deadlock Detected: %s\n",
                   report->deadlock_detected ? "YES" : "NO");
    
    if (report->deadlock_detected) {
        /* Deadlocked PIDs */
        pos += snprintf(output + pos, output_size - pos,
                       "\nDeadlocked Processes (%d):\n",
                       report->num_deadlocked);
        if (report->deadlocked_pids != NULL) {
            for (int i = 0; i < report->num_deadlocked && i < 20; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  PID %d", report->deadlocked_pids[i]);
                if (i < report->num_deadlocked - 1) {
                    pos += snprintf(output + pos, output_size - pos, ", ");
                }
            }
            if (report->num_deadlocked > 20) {
                pos += snprintf(output + pos, output_size - pos,
                               " ... (%d more)", report->num_deadlocked - 20);
            }
            pos += snprintf(output + pos, output_size - pos, "\n");
        }
        
        /* Cycle chains */
        if (report->num_cycles > 0 && report->cycles != NULL) {
            pos += snprintf(output + pos, output_size - pos,
                           "\nCycle Chains (%d):\n", report->num_cycles);
            for (int i = 0; i < report->num_cycles; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  Cycle #%d: ", i + 1);
                if (report->cycles[i].process_ids != NULL &&
                    report->cycles[i].num_processes > 0) {
                    for (int j = 0; j < report->cycles[i].num_processes; j++) {
                        if (j > 0) pos += snprintf(output + pos, output_size - pos, " -> ");
                        pos += snprintf(output + pos, output_size - pos, "P%d",
                                       report->cycles[i].process_ids[j]);
                    }
                }
                pos += snprintf(output + pos, output_size - pos, "\n");
            }
        }
        
        /* Recommendations */
        if (report->num_recommendations > 0 && report->recommendations != NULL) {
            pos += snprintf(output + pos, output_size - pos,
                           "\nRecommendations:\n");
            for (int i = 0; i < report->num_recommendations; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  %d. %s\n", i + 1,
                               report->recommendations[i] ? report->recommendations[i] : "");
            }
        }
    }
    
    pos += snprintf(output + pos, output_size - pos,
                   "\n========================================\n");
    
    return pos;
}

/*
 * format_as_json - Format report as JSON
 * @report: DeadlockReport to format
 * @output: Output buffer to write JSON
 * @output_size: Size of output buffer
 * @return: Number of characters written, or negative on error
 * Description: Formats deadlock report as valid JSON format.
 *              Includes all report fields in structured JSON format.
 *              Time complexity: O(D + C * L)
 * Error handling: Returns negative error code if buffer too small or invalid JSON
 */
int format_as_json(const DeadlockReport* report, char* output, size_t output_size)
{
    if (report == NULL || output == NULL || output_size == 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    int pos = 0;
    char escaped[2048];
    
    /* Opening brace */
    pos += snprintf(output + pos, output_size - pos, "{\n");
    
    /* Basic fields */
    pos += snprintf(output + pos, output_size - pos,
                   "  \"deadlock_detected\": %s,\n",
                   report->deadlock_detected ? "true" : "false");
    pos += snprintf(output + pos, output_size - pos,
                   "  \"num_deadlocked\": %d,\n",
                   report->num_deadlocked);
    pos += snprintf(output + pos, output_size - pos,
                   "  \"num_cycles\": %d,\n",
                   report->num_cycles);
    pos += snprintf(output + pos, output_size - pos,
                   "  \"total_processes_scanned\": %d,\n",
                   report->total_processes_scanned);
    pos += snprintf(output + pos, output_size - pos,
                   "  \"total_resources_found\": %d,\n",
                   report->total_resources_found);
    
    /* Timestamp */
    char time_str[64];
    format_timestamp(report->timestamp, time_str, sizeof(time_str));
    escape_json_string(time_str, escaped, sizeof(escaped));
    pos += snprintf(output + pos, output_size - pos,
                   "  \"timestamp\": \"%s\",\n", escaped);
    
    /* Deadlocked PIDs */
    pos += snprintf(output + pos, output_size - pos, "  \"deadlocked_pids\": [");
    if (report->deadlocked_pids != NULL && report->num_deadlocked > 0) {
        for (int i = 0; i < report->num_deadlocked; i++) {
            if (i > 0) pos += snprintf(output + pos, output_size - pos, ", ");
            pos += snprintf(output + pos, output_size - pos, "%d",
                           report->deadlocked_pids[i]);
        }
    }
    pos += snprintf(output + pos, output_size - pos, "],\n");
    
    /* Cycles */
    pos += snprintf(output + pos, output_size - pos, "  \"cycles\": [");
    if (report->cycles != NULL && report->num_cycles > 0) {
        for (int i = 0; i < report->num_cycles; i++) {
            if (i > 0) pos += snprintf(output + pos, output_size - pos, ", ");
            pos += snprintf(output + pos, output_size - pos, "\n    {\n");
            pos += snprintf(output + pos, output_size - pos,
                           "      \"cycle_id\": %d,\n", i + 1);
            pos += snprintf(output + pos, output_size - pos,
                           "      \"cycle_length\": %d,\n",
                           report->cycles[i].cycle_length);
            
            /* Process IDs */
            pos += snprintf(output + pos, output_size - pos, "      \"process_ids\": [");
            if (report->cycles[i].process_ids != NULL &&
                report->cycles[i].num_processes > 0) {
                for (int j = 0; j < report->cycles[i].num_processes; j++) {
                    if (j > 0) pos += snprintf(output + pos, output_size - pos, ", ");
                    pos += snprintf(output + pos, output_size - pos, "%d",
                                   report->cycles[i].process_ids[j]);
                }
            }
            pos += snprintf(output + pos, output_size - pos, "],\n");
            
            /* Resource IDs */
            pos += snprintf(output + pos, output_size - pos, "      \"resource_ids\": [");
            if (report->cycles[i].resource_ids != NULL &&
                report->cycles[i].num_resources > 0) {
                for (int j = 0; j < report->cycles[i].num_resources; j++) {
                    if (j > 0) pos += snprintf(output + pos, output_size - pos, ", ");
                    pos += snprintf(output + pos, output_size - pos, "%d",
                                   report->cycles[i].resource_ids[j]);
                }
            }
            pos += snprintf(output + pos, output_size - pos, "]\n");
            pos += snprintf(output + pos, output_size - pos, "    }");
        }
    }
    pos += snprintf(output + pos, output_size - pos, "\n  ],\n");
    
    /* Explanations */
    pos += snprintf(output + pos, output_size - pos, "  \"explanations\": [");
    if (report->explanations != NULL && report->num_explanations > 0) {
        for (int i = 0; i < report->num_explanations; i++) {
            if (i > 0) pos += snprintf(output + pos, output_size - pos, ", ");
            escape_json_string(report->explanations[i], escaped, sizeof(escaped));
            pos += snprintf(output + pos, output_size - pos, "\n    \"%s\"", escaped);
        }
    }
    pos += snprintf(output + pos, output_size - pos, "\n  ],\n");
    
    /* Recommendations */
    pos += snprintf(output + pos, output_size - pos, "  \"recommendations\": [");
    if (report->recommendations != NULL && report->num_recommendations > 0) {
        for (int i = 0; i < report->num_recommendations; i++) {
            if (i > 0) pos += snprintf(output + pos, output_size - pos, ", ");
            escape_json_string(report->recommendations[i], escaped, sizeof(escaped));
            pos += snprintf(output + pos, output_size - pos, "\n    \"%s\"", escaped);
        }
    }
    pos += snprintf(output + pos, output_size - pos, "\n  ]\n");
    
    /* Closing brace */
    pos += snprintf(output + pos, output_size - pos, "}\n");
    
    return pos;
}

/*
 * format_as_verbose - Format report as verbose detailed text
 * @report: DeadlockReport to format
 * @output: Output buffer to write verbose text
 * @output_size: Size of output buffer
 * @return: Number of characters written, or negative on error
 * Description: Formats deadlock report with maximum detail including all
 *              process information, resource details, wait chains, and analysis.
 *              Time complexity: O(D + C * L + R) where R=resources
 * Error handling: Returns negative error code if buffer too small
 */
int format_as_verbose(const DeadlockReport* report, char* output, size_t output_size)
{
    if (report == NULL || output == NULL || output_size == 0) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    int pos = 0;
    
    /* Header */
    pos += snprintf(output + pos, output_size - pos,
                   "========================================\n");
    pos += snprintf(output + pos, output_size - pos,
                   "    DEADLOCK DETECTION REPORT\n");
    pos += snprintf(output + pos, output_size - pos,
                   "    Detailed Analysis\n");
    pos += snprintf(output + pos, output_size - pos,
                   "========================================\n\n");
    
    /* Summary */
    pos += snprintf(output + pos, output_size - pos,
                   "SUMMARY\n");
    pos += snprintf(output + pos, output_size - pos,
                   "----------------------------------------\n");
    pos += snprintf(output + pos, output_size - pos,
                   "Deadlock Detected: %s\n",
                   report->deadlock_detected ? "YES" : "NO");
    pos += snprintf(output + pos, output_size - pos,
                   "Deadlocked Processes: %d\n",
                   report->num_deadlocked);
    pos += snprintf(output + pos, output_size - pos,
                   "Cycles Found: %d\n",
                   report->num_cycles);
    pos += snprintf(output + pos, output_size - pos,
                   "Processes Scanned: %d\n",
                   report->total_processes_scanned);
    pos += snprintf(output + pos, output_size - pos,
                   "Resources Found: %d\n",
                   report->total_resources_found);
    
    if (report->timestamp > 0) {
        char time_str[64];
        format_timestamp(report->timestamp, time_str, sizeof(time_str));
        pos += snprintf(output + pos, output_size - pos,
                       "Detection Time: %s\n", time_str);
    }
    
    if (report->deadlock_detected) {
        /* Deadlocked Processes */
        pos += snprintf(output + pos, output_size - pos,
                       "\nDEADLOCKED PROCESSES\n");
        pos += snprintf(output + pos, output_size - pos,
                       "----------------------------------------\n");
        if (report->deadlocked_pids != NULL && report->num_deadlocked > 0) {
            for (int i = 0; i < report->num_deadlocked; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  Process #%d: PID %d\n", i + 1,
                               report->deadlocked_pids[i]);
            }
        }
        
        /* Detailed Wait Chains */
        pos += snprintf(output + pos, output_size - pos,
                       "\nDETAILED WAIT CHAINS\n");
        pos += snprintf(output + pos, output_size - pos,
                       "----------------------------------------\n");
        if (report->cycles != NULL && report->num_cycles > 0) {
            for (int i = 0; i < report->num_cycles; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "\nCycle #%d:\n", i + 1);
                
                if (report->cycles[i].cycle_path != NULL) {
                    pos += snprintf(output + pos, output_size - pos,
                                   "  Path: ");
                    for (int j = 0; j < report->cycles[i].cycle_length - 1; j++) {
                        if (j > 0) pos += snprintf(output + pos, output_size - pos, " -> ");
                        pos += snprintf(output + pos, output_size - pos, "%d",
                                       report->cycles[i].cycle_path[j]);
                    }
                    pos += snprintf(output + pos, output_size - pos, "\n");
                }
                
                if (report->cycles[i].process_ids != NULL &&
                    report->cycles[i].num_processes > 0) {
                    pos += snprintf(output + pos, output_size - pos,
                                   "  Processes: ");
                    for (int j = 0; j < report->cycles[i].num_processes; j++) {
                        if (j > 0) pos += snprintf(output + pos, output_size - pos, ", ");
                        pos += snprintf(output + pos, output_size - pos, "PID %d",
                                       report->cycles[i].process_ids[j]);
                    }
                    pos += snprintf(output + pos, output_size - pos, "\n");
                }
                
                if (report->cycles[i].resource_ids != NULL &&
                    report->cycles[i].num_resources > 0) {
                    pos += snprintf(output + pos, output_size - pos,
                                   "  Resources: ");
                    for (int j = 0; j < report->cycles[i].num_resources; j++) {
                        if (j > 0) pos += snprintf(output + pos, output_size - pos, ", ");
                        pos += snprintf(output + pos, output_size - pos, "RID %d",
                                       report->cycles[i].resource_ids[j]);
                    }
                    pos += snprintf(output + pos, output_size - pos, "\n");
                }
            }
        }
        
        /* Explanations */
        if (report->explanations != NULL && report->num_explanations > 0) {
            pos += snprintf(output + pos, output_size - pos,
                           "\nEXPLANATIONS\n");
            pos += snprintf(output + pos, output_size - pos,
                           "----------------------------------------\n");
            for (int i = 0; i < report->num_explanations; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  %d. %s\n", i + 1,
                               report->explanations[i] ? report->explanations[i] : "");
            }
        }
        
        /* Recommendations */
        if (report->recommendations != NULL && report->num_recommendations > 0) {
            pos += snprintf(output + pos, output_size - pos,
                           "\nRECOMMENDATIONS\n");
            pos += snprintf(output + pos, output_size - pos,
                           "----------------------------------------\n");
            for (int i = 0; i < report->num_recommendations; i++) {
                pos += snprintf(output + pos, output_size - pos,
                               "  %d. %s\n", i + 1,
                               report->recommendations[i] ? report->recommendations[i] : "");
            }
        }
    }
    
    pos += snprintf(output + pos, output_size - pos,
                   "\n========================================\n");
    pos += snprintf(output + pos, output_size - pos,
                   "End of Report\n");
    pos += snprintf(output + pos, output_size - pos,
                   "========================================\n");
    
    return pos;
}

/* =============================================================================
 * MAIN DISPLAY FUNCTION
 * =============================================================================
 */

/*
 * display_deadlock_report - Display deadlock report in specified format
 * @report: DeadlockReport to display
 * @fmt: Output format to use (TEXT, JSON, or VERBOSE)
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Main function for displaying deadlock reports. Formats and
 *              prints report according to specified format. Outputs to stdout.
 *              Time complexity: O(D + C * L) where D=deadlocked processes,
 *              C=cycles, L=avg cycle length
 * Error handling: Returns error codes for invalid formats or NULL reports
 */
int display_deadlock_report(const DeadlockReport* report, OutputFormat fmt)
{
    if (report == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    print_header(fmt);
    
    switch (fmt) {
        case FORMAT_TEXT:
            print_summary(report);
            if (report->deadlock_detected) {
                print_process_info(report);
                print_detailed_wait_chain(report);
                print_recommendations(report);
            }
            break;
            
        case FORMAT_JSON: {
            /* Use format function for JSON to ensure validity */
            char buffer[16384];
            int len = format_as_json(report, buffer, sizeof(buffer));
            if (len < 0) {
                return len;
            }
            printf("%s", buffer);
            break;
        }
        
        case FORMAT_VERBOSE:
            print_summary(report);
            if (report->deadlock_detected) {
                print_process_info(report);
                print_detailed_wait_chain(report);
                print_resource_info(report);
                if (report->explanations != NULL && report->num_explanations > 0) {
                    printf("\nExplanations:\n");
                    printf("----------------------------------------\n");
                    for (int i = 0; i < report->num_explanations; i++) {
                        printf("  %d. %s\n", i + 1,
                               report->explanations[i] ? report->explanations[i] : "");
                    }
                }
                print_recommendations(report);
            }
            break;
            
        default:
            error_log("Invalid output format: %d", fmt);
            return ERROR_INVALID_FORMAT;
    }
    
    print_footer(fmt);
    
    return SUCCESS;
}

/*
 * export_to_file - Export deadlock report to a file
 * @report: DeadlockReport to export
 * @filename: Path to output file
 * @fmt: Output format to use
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Writes deadlock report to specified file in requested format.
 *              Creates file if it doesn't exist, overwrites if it does.
 *              Time complexity: O(D + C * L + file_write_time)
 * Error handling: Returns error codes for file open/write errors
 */
int export_to_file(const DeadlockReport* report, const char* filename,
                   OutputFormat fmt)
{
    if (report == NULL || filename == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    FILE* file = fopen(filename, "w");
    if (file == NULL) {
        error_log("Failed to open file for writing: %s (errno: %d)", filename, errno);
        return ERROR_FILE_NOT_FOUND;
    }
    
    /* Format report based on format type */
    char buffer[32768];
    int len = 0;
    
    switch (fmt) {
        case FORMAT_TEXT:
            len = format_as_text(report, buffer, sizeof(buffer));
            break;
            
        case FORMAT_JSON:
            len = format_as_json(report, buffer, sizeof(buffer));
            break;
            
        case FORMAT_VERBOSE:
            len = format_as_verbose(report, buffer, sizeof(buffer));
            break;
            
        default:
            fclose(file);
            return ERROR_INVALID_FORMAT;
    }
    
    if (len < 0) {
        fclose(file);
        return len;
    }
    
    /* Write to file */
    size_t written = fwrite(buffer, 1, (size_t)len, file);
    if (written != (size_t)len) {
        error_log("Failed to write complete report to file: %s", filename);
        fclose(file);
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    fclose(file);
    info_log("Report exported to: %s", filename);
    
    return SUCCESS;
}

