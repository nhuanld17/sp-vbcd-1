#ifndef OUTPUT_HANDLER_H
#define OUTPUT_HANDLER_H

/* =============================================================================
 * OUTPUT_HANDLER.H - Output Formatting and Display Interface
 * =============================================================================
 * This header defines structures and functions for formatting and displaying
 * deadlock detection results in various output formats.
 * =============================================================================
 */

#include "deadlock_detection.h"
#include "config.h"

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

/*
 * OutputFormat - Enumeration of supported output formats
 */
typedef enum {
    FORMAT_TEXT = OUTPUT_FORMAT_TEXT,      /* Plain text format */
    FORMAT_JSON = OUTPUT_FORMAT_JSON,      /* JSON format */
    FORMAT_VERBOSE = OUTPUT_FORMAT_VERBOSE  /* Verbose detailed format */
} OutputFormat;

/* =============================================================================
 * FUNCTION PROTOTYPES
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
int display_deadlock_report(const DeadlockReport* report, OutputFormat fmt);

/*
 * print_summary - Print summary of deadlock detection results
 * @report: DeadlockReport to summarize
 * @return: None
 * Description: Prints brief summary including number of deadlocked processes,
 *              number of cycles detected, and timestamp.
 *              Time complexity: O(1)
 * Error handling: Handles NULL report safely
 */
void print_summary(const DeadlockReport* report);

/*
 * print_detailed_wait_chain - Print detailed wait chain for deadlock
 * @report: DeadlockReport containing cycle information
 * @return: None
 * Description: Prints detailed wait chain showing how processes are waiting
 *              for each other in a cycle format: P1->R1->P2->R2->P1
 *              Time complexity: O(C * L) where C=cycles, L=avg cycle length
 * Error handling: Handles NULL report and empty cycles safely
 */
void print_detailed_wait_chain(const DeadlockReport* report);

/*
 * print_process_info - Print detailed information about deadlocked processes
 * @report: DeadlockReport containing process information
 * @return: None
 * Description: Prints detailed information for each deadlocked process including
 *              PID, name, and resources involved.
 *              Time complexity: O(D) where D=deadlocked processes
 * Error handling: Handles NULL report and missing process info gracefully
 */
void print_process_info(const DeadlockReport* report);

/*
 * print_resource_info - Print information about resources in deadlock
 * @report: DeadlockReport containing resource information
 * @return: None
 * Description: Prints information about resources involved in deadlock cycles,
 *              including resource IDs and which processes hold/wait for them.
 *              Time complexity: O(R) where R=resources in cycles
 * Error handling: Handles NULL report safely
 */
void print_resource_info(const DeadlockReport* report);

/*
 * print_recommendations - Print recommendations for resolving deadlocks
 * @report: DeadlockReport containing recommendations
 * @return: None
 * Description: Prints actionable recommendations for resolving detected deadlocks,
 *              such as which processes to terminate or resources to release.
 *              Time complexity: O(N) where N=number of recommendations
 * Error handling: Handles NULL report and missing recommendations safely
 */
void print_recommendations(const DeadlockReport* report);

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
                   OutputFormat fmt);

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
int format_as_text(const DeadlockReport* report, char* output, size_t output_size);

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
int format_as_json(const DeadlockReport* report, char* output, size_t output_size);

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
int format_as_verbose(const DeadlockReport* report, char* output, size_t output_size);

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
int parse_output_format(const char* format_str, OutputFormat* fmt);

/*
 * get_format_string - Get string representation of output format
 * @fmt: OutputFormat enum value
 * @return: String name of format, or NULL for invalid format
 * Description: Returns human-readable string for output format enum.
 *              Time complexity: O(1)
 * Error handling: Returns NULL for invalid enum values
 */
const char* get_format_string(OutputFormat fmt);

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
void print_header(OutputFormat format);

/*
 * print_footer - Print formatted footer for deadlock report
 * @format: Output format to use for footer styling
 * @return: None
 * Description: Prints a formatted footer appropriate for the output format.
 *              Closes JSON structure, adds summary, etc.
 *              Time complexity: O(1)
 * Error handling: Handles invalid formats gracefully
 */
void print_footer(OutputFormat format);

#endif /* OUTPUT_HANDLER_H */

