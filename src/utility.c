/* =============================================================================
 * UTILITY.C - Utility Functions Implementation
 * =============================================================================
 * Implementation of common utility functions for memory management,
 * string operations, file operations, and error handling.
 * =============================================================================
 */

#include "utility.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <ctype.h>

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
void* safe_malloc(size_t size)
{
    void* ptr = malloc(size);
    
    if (!ptr) {
        error_log("Out of memory: failed to allocate %zu bytes", size);
        errno = ENOMEM;
        return NULL;
    }
    
    return ptr;
}

/*
 * safe_realloc - Reallocate memory with error checking
 * @ptr: Pointer to previously allocated memory
 * @size: New size in bytes
 * @return: Pointer to reallocated memory, or NULL on failure
 * Description: Wrapper around realloc() that checks for failure.
 *              Preserves original pointer if reallocation fails.
 * Error handling: Returns NULL if reallocation fails, logs error
 */
void* safe_realloc(void* ptr, size_t size)
{
    void* new_ptr = realloc(ptr, size);
    
    if (!new_ptr && size > 0) {
        error_log("Out of memory: failed to reallocate %zu bytes", size);
        errno = ENOMEM;
        /* Original pointer is preserved by realloc on failure */
        return NULL;
    }
    
    return new_ptr;
}

/*
 * safe_free - Free memory and set pointer to NULL
 * @ptr: Pointer to pointer that should be freed
 * @return: None
 * Description: Safely frees memory and sets pointer to NULL to prevent
 *              use-after-free errors. Accepts NULL safely.
 * Error handling: Handles NULL pointers gracefully
 */
void safe_free(void** ptr)
{
    if (ptr != NULL && *ptr != NULL) {
        free(*ptr);
        *ptr = NULL;
    }
}

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
char* str_trim(char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    /* Find start of non-whitespace */
    char* start = str;
    while (*start != '\0' && isspace((unsigned char)*start)) {
        start++;
    }
    
    /* If entire string is whitespace */
    if (*start == '\0') {
        *str = '\0';
        return str;
    }
    
    /* Find end of non-whitespace */
    char* end = start;
    char* last_non_space = start;
    while (*end != '\0') {
        if (!isspace((unsigned char)*end)) {
            last_non_space = end;
        }
        end++;
    }
    
    /* Null-terminate after last non-whitespace */
    *(last_non_space + 1) = '\0';
    
    /* Move trimmed string to start if needed */
    if (start != str) {
        size_t len = strlen(start) + 1;
        memmove(str, start, len);
    }
    
    return str;
}

/*
 * str_starts_with - Check if string starts with a prefix
 * @str: String to check
 * @prefix: Prefix to search for
 * @return: 1 if string starts with prefix, 0 otherwise
 * Description: Case-sensitive prefix matching. Time complexity: O(m)
 *              where m is length of prefix.
 * Error handling: Returns 0 if either parameter is NULL
 */
int str_starts_with(const char* str, const char* prefix)
{
    if (str == NULL || prefix == NULL) {
        return 0;
    }
    
    while (*prefix != '\0') {
        if (*str != *prefix) {
            return 0;
        }
        str++;
        prefix++;
    }
    
    return 1;
}

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
char** str_split(const char* str, char delim, int* count)
{
    if (str == NULL || count == NULL) {
        if (count != NULL) {
            *count = 0;
        }
        return NULL;
    }
    
    *count = 0;
    
    /* First pass: count tokens */
    const char* p = str;
    int token_count = 0;
    int in_token = 0;
    
    while (*p != '\0') {
        if (*p == delim) {
            if (in_token) {
                token_count++;
                in_token = 0;
            }
        } else {
            in_token = 1;
        }
        p++;
    }
    
    /* Count last token if string doesn't end with delimiter */
    if (in_token) {
        token_count++;
    }
    
    /* Handle empty string or only delimiters */
    if (token_count == 0) {
        *count = 0;
        char** result = (char**)safe_malloc(sizeof(char*));
        if (result == NULL) {
            return NULL;
        }
        result[0] = NULL;
        return result;
    }
    
    /* Allocate array for pointers */
    char** tokens = (char**)safe_malloc(sizeof(char*) * (token_count + 1));
    if (tokens == NULL) {
        *count = 0;
        return NULL;
    }
    
    /* Second pass: extract tokens */
    p = str;
    const char* token_start = NULL;
    int token_idx = 0;
    in_token = 0;
    
    while (*p != '\0') {
        if (*p == delim) {
            if (in_token) {
                /* End of token */
                size_t token_len = p - token_start;
                tokens[token_idx] = (char*)safe_malloc(token_len + 1);
                if (tokens[token_idx] == NULL) {
                    /* Cleanup on failure */
                    free_str_array(tokens, token_idx);
                    *count = 0;
                    return NULL;
                }
                memcpy(tokens[token_idx], token_start, token_len);
                tokens[token_idx][token_len] = '\0';
                token_idx++;
                in_token = 0;
            }
        } else {
            if (!in_token) {
                token_start = p;
                in_token = 1;
            }
        }
        p++;
    }
    
    /* Handle last token */
    if (in_token) {
        size_t token_len = p - token_start;
        tokens[token_idx] = (char*)safe_malloc(token_len + 1);
        if (tokens[token_idx] == NULL) {
            free_str_array(tokens, token_idx);
            *count = 0;
            return NULL;
        }
        memcpy(tokens[token_idx], token_start, token_len);
        tokens[token_idx][token_len] = '\0';
        token_idx++;
    }
    
    tokens[token_idx] = NULL; /* Sentinel */
    *count = token_idx;
    
    return tokens;
}

/*
 * free_str_array - Free array of strings allocated by str_split
 * @arr: Array of strings to free
 * @count: Number of strings in array
 * @return: None
 * Description: Frees each string in array and the array itself.
 *              Handles NULL arrays safely.
 * Error handling: Handles NULL arrays and invalid counts gracefully
 */
void free_str_array(char** arr, int count)
{
    if (arr == NULL) {
        return;
    }
    
    /* Free each string */
    for (int i = 0; i < count; i++) {
        if (arr[i] != NULL) {
            free(arr[i]);
            arr[i] = NULL;
        }
    }
    
    /* Free the array itself */
    free(arr);
}

/*
 * str_dup - Duplicate a string
 * @str: String to duplicate
 * @return: Newly allocated copy of string, or NULL on failure
 * Description: Creates a copy of the input string using safe_malloc.
 *              Caller must free the result. Time complexity: O(n)
 * Error handling: Returns NULL if allocation fails or input is NULL
 */
char* str_dup(const char* str)
{
    if (str == NULL) {
        return NULL;
    }
    
    size_t len = strlen(str) + 1;
    char* copy = (char*)safe_malloc(len);
    if (copy == NULL) {
        return NULL;
    }
    
    memcpy(copy, str, len);
    return copy;
}

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
int file_exists(const char* path)
{
    if (path == NULL) {
        return 0;
    }
    
    return (access(path, F_OK) == 0) ? 1 : 0;
}

/*
 * read_entire_file - Read entire file into a string
 * @path: Path to file
 * @return: Allocated string containing file contents, or NULL on error
 * Description: Reads entire file into memory. Caller must free result.
 *              Handles files up to MAX_LINE_LEN * 1000 lines.
 *              Time complexity: O(n) where n is file size.
 * Error handling: Returns NULL on file open error, read error, or allocation failure
 */
char* read_entire_file(const char* path)
{
    if (path == NULL) {
        error_log("read_entire_file: path is NULL");
        return NULL;
    }
    
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        error_log("Failed to open file: %s (errno: %d)", path, errno);
        return NULL;
    }
    
    /* Get file size */
    if (fseek(file, 0, SEEK_END) != 0) {
        error_log("Failed to seek to end of file: %s", path);
        fclose(file);
        return NULL;
    }
    
    long file_size = ftell(file);
    if (file_size < 0) {
        error_log("Failed to get file size: %s", path);
        fclose(file);
        return NULL;
    }
    
    if (fseek(file, 0, SEEK_SET) != 0) {
        error_log("Failed to seek to start of file: %s", path);
        fclose(file);
        return NULL;
    }
    
    /* Allocate buffer (add 1 for null terminator) */
    size_t buffer_size = (size_t)file_size + 1;
    char* buffer = (char*)safe_malloc(buffer_size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }
    
    /* Read file contents */
    size_t bytes_read = fread(buffer, 1, (size_t)file_size, file);
    if (bytes_read != (size_t)file_size && ferror(file)) {
        error_log("Failed to read file: %s (read %zu of %ld bytes)", 
                  path, bytes_read, file_size);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[bytes_read] = '\0'; /* Null-terminate */
    
    fclose(file);
    return buffer;
}

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
char* read_proc_file_safe(int pid, const char* filename)
{
    if (filename == NULL) {
        error_log("read_proc_file_safe: filename is NULL");
        return NULL;
    }
    
    char path[MAX_PATH_LEN];
    int result;
    
    if (pid > 0) {
        /* Process-specific file: /proc/[PID]/filename */
        result = snprintf(path, sizeof(path), "%s/%d/%s", 
                         PROC_BASE_PATH, pid, filename);
    } else {
        /* System-wide file: /proc/filename */
        result = snprintf(path, sizeof(path), "%s/%s", 
                         PROC_BASE_PATH, filename);
    }
    
    if (result < 0 || (size_t)result >= sizeof(path)) {
        error_log("read_proc_file_safe: path too long for PID %d, file %s", 
                  pid, filename);
        return NULL;
    }
    
    FILE* file = fopen(path, "r");
    if (file == NULL) {
        if (errno == ENOENT) {
            /* Process terminated or file doesn't exist - not an error */
            debug_log("File not found (process may have terminated): %s", path);
            return NULL;
        } else if (errno == EACCES) {
            error_log("Permission denied: %s", path);
            return NULL;
        } else {
            error_log("Failed to open file: %s (errno: %d)", path, errno);
            return NULL;
        }
    }
    
    /* Read file line by line to handle large files */
    size_t buffer_size = MAX_LINE_LEN;
    size_t buffer_pos = 0;
    char* buffer = (char*)safe_malloc(buffer_size);
    if (buffer == NULL) {
        fclose(file);
        return NULL;
    }
    
    char line[MAX_LINE_LEN];
    while (fgets(line, sizeof(line), file) != NULL) {
        size_t line_len = strlen(line);
        
        /* Check if we need to expand buffer */
        if (buffer_pos + line_len + 1 > buffer_size) {
            size_t new_size = buffer_size * 2;
            if (new_size < buffer_pos + line_len + 1) {
                new_size = buffer_pos + line_len + 1;
            }
            
            char* new_buffer = (char*)safe_realloc(buffer, new_size);
            if (new_buffer == NULL) {
                free(buffer);
                fclose(file);
                return NULL;
            }
            buffer = new_buffer;
            buffer_size = new_size;
        }
        
        memcpy(buffer + buffer_pos, line, line_len);
        buffer_pos += line_len;
    }
    
    if (ferror(file)) {
        error_log("Error reading file: %s", path);
        free(buffer);
        fclose(file);
        return NULL;
    }
    
    buffer[buffer_pos] = '\0';
    fclose(file);
    
    return buffer;
}

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
void error_exit(const char* msg, int code)
{
    if (msg != NULL) {
        fprintf(stderr, "FATAL ERROR: %s\n", msg);
    } else {
        fprintf(stderr, "FATAL ERROR: Unknown error\n");
    }
    exit(code);
}

/*
 * print_error - Print error message to stderr
 * @msg: Error message to print
 * @return: None
 * Description: Prints error message with formatting. Does not exit.
 * Error handling: Handles NULL message safely
 */
void print_error(const char* msg)
{
    if (msg != NULL) {
        fprintf(stderr, "Error: %s\n", msg);
    } else {
        fprintf(stderr, "Error: Unknown error\n");
    }
}

/*
 * format_error_string - Format error message with errno information
 * @operation: Operation that failed (e.g., "open file")
 * @path: Path or identifier related to error
 * @return: Formatted error string (caller must free)
 * Description: Creates descriptive error message including errno details.
 *              Time complexity: O(1)
 * Error handling: Returns NULL on allocation failure
 */
char* format_error_string(const char* operation, const char* path)
{
    if (operation == NULL) {
        operation = "unknown operation";
    }
    
    if (path == NULL) {
        path = "unknown path";
    }
    
    char* error_msg = strerror(errno);
    size_t msg_len = strlen(operation) + strlen(path) + strlen(error_msg) + 50;
    char* formatted = (char*)safe_malloc(msg_len);
    
    if (formatted == NULL) {
        return NULL;
    }
    
    int result = snprintf(formatted, msg_len, 
                          "%s '%s': %s (errno: %d)",
                          operation, path, error_msg, errno);
    
    if (result < 0 || (size_t)result >= msg_len) {
        /* snprintf failed or truncated - use fallback */
        snprintf(formatted, msg_len, 
                 "%s '%s': %s", operation, path, error_msg);
    }
    
    return formatted;
}

