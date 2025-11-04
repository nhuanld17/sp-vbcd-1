#ifndef UTILITY_H
#define UTILITY_H

/* =============================================================================
 * UTILITY.H - Utility Functions and Macros
 * =============================================================================
 * This header provides common utility functions for memory management,
 * string operations, file operations, and error handling used throughout
 * the deadlock detection system.
 * =============================================================================
 */

#include <stddef.h>
#include <stdio.h>

/* =============================================================================
 * LOGGING MACROS
 * =============================================================================
 * These macros provide consistent logging across the codebase.
 * error_log outputs to stderr, info_log outputs to stdout.
 */

#if DEBUG
#define debug_log(fmt, ...) \
    fprintf(stderr, "[DEBUG %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define debug_log(fmt, ...)
#endif

#define error_log(fmt, ...) \
    fprintf(stderr, "[ERROR %s:%d] " fmt "\n", __FILE__, __LINE__, ##__VA_ARGS__)

#define info_log(fmt, ...) \
    fprintf(stdout, "[INFO] " fmt "\n", ##__VA_ARGS__)

/* =============================================================================
 * MEMORY MANAGEMENT FUNCTIONS
 * =============================================================================
 */

/*
 * safe_malloc - Allocate memory with error checking
 * @size: Number of bytes to allocate
 * @return: Pointer to allocated memory, or NULL on failure
 * Description: Wrapper around malloc() that checks for allocation failure.
 *              If allocation fails, logs error and returns NULL.
 * Error handling: Returns NULL if allocation fails, logs error to stderr
 */
void* safe_malloc(size_t size);

/*
 * safe_realloc - Reallocate memory with error checking
 * @ptr: Pointer to previously allocated memory
 * @size: New size in bytes
 * @return: Pointer to reallocated memory, or NULL on failure
 * Description: Wrapper around realloc() that checks for failure.
 *              Preserves original pointer if reallocation fails.
 * Error handling: Returns NULL if reallocation fails, logs error
 */
void* safe_realloc(void* ptr, size_t size);

/*
 * safe_free - Free memory and set pointer to NULL
 * @ptr: Pointer to pointer that should be freed
 * @return: None
 * Description: Safely frees memory and sets pointer to NULL to prevent
 *              use-after-free errors. Accepts NULL safely.
 * Error handling: Handles NULL pointers gracefully
 */
void safe_free(void** ptr);

/* =============================================================================
 * STRING UTILITY FUNCTIONS
 * =============================================================================
 */

/*
 * str_trim - Remove leading and trailing whitespace from string
 * @str: String to trim (modified in place)
 * @return: Pointer to trimmed string (same as input)
 * Description: Removes spaces, tabs, newlines from start and end of string.
 *              Modifies string in place. Time complexity: O(n)
 * Error handling: Returns NULL if input is NULL
 */
char* str_trim(char* str);

/*
 * str_starts_with - Check if string starts with a prefix
 * @str: String to check
 * @prefix: Prefix to search for
 * @return: 1 if string starts with prefix, 0 otherwise
 * Description: Case-sensitive prefix matching. Time complexity: O(m)
 *              where m is length of prefix.
 * Error handling: Returns 0 if either parameter is NULL
 */
int str_starts_with(const char* str, const char* prefix);

/*
 * str_split - Split string by delimiter into array of strings
 * @str: String to split
 * @delim: Delimiter character
 * @count: Output parameter for number of tokens found
 * @return: Array of strings (must be freed with free_str_array), or NULL on error
 * Description: Splits string by delimiter and returns array of tokens.
 *              Memory is allocated for result array and each token.
 *              Time complexity: O(n) where n is string length.
 * Error handling: Returns NULL on allocation failure, sets count to 0
 */
char** str_split(const char* str, char delim, int* count);

/*
 * free_str_array - Free array of strings allocated by str_split
 * @arr: Array of strings to free
 * @count: Number of strings in array
 * @return: None
 * Description: Frees each string in array and the array itself.
 *              Handles NULL arrays safely.
 * Error handling: Handles NULL arrays and invalid counts gracefully
 */
void free_str_array(char** arr, int count);

/*
 * str_dup - Duplicate a string
 * @str: String to duplicate
 * @return: Newly allocated copy of string, or NULL on failure
 * Description: Creates a copy of the input string using safe_malloc.
 *              Caller must free the result. Time complexity: O(n)
 * Error handling: Returns NULL if allocation fails or input is NULL
 */
char* str_dup(const char* str);

/* =============================================================================
 * FILE UTILITY FUNCTIONS
 * =============================================================================
 */

/*
 * file_exists - Check if a file exists and is accessible
 * @path: Path to file
 * @return: 1 if file exists, 0 otherwise
 * Description: Checks file existence using access() system call.
 *              Time complexity: O(1) system call
 * Error handling: Returns 0 on any error (including permission denied)
 */
int file_exists(const char* path);

/*
 * read_entire_file - Read entire file into a string
 * @path: Path to file
 * @return: Allocated string containing file contents, or NULL on error
 * Description: Reads entire file into memory. Caller must free result.
 *              Handles files up to MAX_LINE_LEN * 1000 lines.
 *              Time complexity: O(n) where n is file size.
 * Error handling: Returns NULL on file open error, read error, or allocation failure
 */
char* read_entire_file(const char* path);

/*
 * read_proc_file_safe - Safely read a /proc file
 * @pid: Process ID (0 for system-wide files)
 * @filename: Name of file in /proc or /proc/[PID]/
 * @return: Allocated string with file contents, or NULL on error
 * Description: Reads /proc/[PID]/filename or /proc/filename safely.
 *              Handles process termination (ENOENT) gracefully.
 *              Time complexity: O(file_size)
 * Error handling: Returns NULL on error, handles ENOENT and EACCES
 */
char* read_proc_file_safe(int pid, const char* filename);

/* =============================================================================
 * ERROR HANDLING FUNCTIONS
 * =============================================================================
 */

/*
 * error_exit - Print error message and exit with code
 * @msg: Error message to print
 * @code: Exit code to use
 * @return: Never returns (calls exit())
 * Description: Prints error message to stderr and terminates program.
 *              Used for fatal errors that cannot be recovered.
 * Error handling: Always terminates program
 */
void error_exit(const char* msg, int code);

/*
 * print_error - Print error message to stderr
 * @msg: Error message to print
 * @return: None
 * Description: Prints error message with formatting. Does not exit.
 * Error handling: Handles NULL message safely
 */
void print_error(const char* msg);

/*
 * format_error_string - Format error message with errno information
 * @operation: Operation that failed (e.g., "open file")
 * @path: Path or identifier related to error
 * @return: Formatted error string (caller must free)
 * Description: Creates descriptive error message including errno details.
 *              Time complexity: O(1)
 * Error handling: Returns NULL on allocation failure
 */
char* format_error_string(const char* operation, const char* path);

#endif /* UTILITY_H */

