/* =============================================================================
 * PROCESS_MONITOR.C - Process Monitoring Implementation
 * =============================================================================
 * Implementation of functions to collect process information from the /proc
 * filesystem on Linux systems.
 * =============================================================================
 */

#include "process_monitor.h"
#include "utility.h"
#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <ctype.h>
#include <time.h>

/* Ensure dirent types are available */
#ifndef DT_DIR
#define DT_DIR 4
#endif
#ifndef DT_LNK
#define DT_LNK 10
#endif

/* =============================================================================
 * CACHE STRUCTURE
 * =============================================================================
 */

/*
 * CacheEntry - Cache entry for process information
 */
typedef struct {
    pid_t pid;                      /* Process ID */
    char* status_content;            /* Cached status file content */
    time_t timestamp;                /* Cache timestamp */
    int valid;                      /* 1 if cache entry is valid */
} CacheEntry;

static CacheEntry* s_cache = NULL;  /* Static cache array */
static int s_cache_size = 0;        /* Current cache size */
static int s_cache_capacity = 0;    /* Cache capacity */

#define CACHE_INITIAL_CAPACITY 100
#define CACHE_TTL_SECONDS 5

/* =============================================================================
 * CACHE HELPER FUNCTIONS
 * =============================================================================
 */

/*
 * find_cache_entry - Find cache entry for a PID
 * @pid: Process ID to find
 * @return: Index of cache entry, or -1 if not found
 */
static int find_cache_entry(pid_t pid)
{
    for (int i = 0; i < s_cache_size; i++) {
        if (s_cache[i].valid && s_cache[i].pid == pid) {
            return i;
        }
    }
    return -1;
}

/*
 * get_cache_entry - Get or create cache entry for PID
 * @pid: Process ID
 * @return: Pointer to cache entry, or NULL on error
 */
static CacheEntry* get_cache_entry(pid_t pid)
{
    time_t now = time(NULL);
    
    /* Find existing entry */
    int idx = find_cache_entry(pid);
    if (idx >= 0) {
        /* Check if cache is still valid */
        if (now - s_cache[idx].timestamp < CACHE_TTL_SECONDS) {
            return &s_cache[idx];
        }
        /* Cache expired, free it */
        safe_free((void**)&s_cache[idx].status_content);
        s_cache[idx].valid = 0;
    }
    
    /* Find empty slot or expand cache */
    if (idx < 0) {
        /* Find empty slot */
        for (idx = 0; idx < s_cache_size; idx++) {
            if (!s_cache[idx].valid) {
                break;
            }
        }
        
        /* Expand cache if needed */
        if (idx >= s_cache_size) {
            if (s_cache_size >= s_cache_capacity) {
                int new_capacity = s_cache_capacity == 0 ? 
                    CACHE_INITIAL_CAPACITY : s_cache_capacity * 2;
                CacheEntry* new_cache = (CacheEntry*)safe_realloc(
                    s_cache, sizeof(CacheEntry) * new_capacity);
                if (new_cache == NULL) {
                    return NULL;
                }
                s_cache = new_cache;
                s_cache_capacity = new_capacity;
            }
            idx = s_cache_size++;
        }
    }
    
    /* Initialize cache entry */
    s_cache[idx].pid = pid;
    s_cache[idx].status_content = NULL;
    s_cache[idx].timestamp = now;
    s_cache[idx].valid = 1;
    
    return &s_cache[idx];
}

/* =============================================================================
 * MAIN FUNCTIONS
 * =============================================================================
 */

/*
 * get_all_processes - Get list of all running process IDs
 * @count: Output parameter for number of processes found
 * @return: Array of PIDs (caller must free with free_process_list), or NULL on error
 * Description: Scans /proc directory to find all process IDs.
 *              Returns array of valid PIDs. Time complexity: O(n) where n is
 *              number of processes. Memory: O(n) for PID array.
 * Error handling: Returns NULL on error, sets count to 0
 */
pid_t* get_all_processes(int* count)
{
    if (count == NULL) {
        error_log("get_all_processes: count is NULL");
        return NULL;
    }
    
    *count = 0;
    
    DIR* proc_dir = opendir(PROC_BASE_PATH);
    if (proc_dir == NULL) {
        error_log("Failed to open /proc directory: %s", strerror(errno));
        return NULL;
    }
    
    /* First pass: count numeric directories */
    struct dirent* entry;
    int pid_count = 0;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            /* Check if directory name is numeric (PID) */
            char* endptr;
            long pid_val = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && pid_val > 0) {
                pid_count++;
            }
        }
    }
    
    if (pid_count == 0) {
        closedir(proc_dir);
        *count = 0;
        return NULL;
    }
    
    /* Allocate array for PIDs */
    pid_t* pids = (pid_t*)safe_malloc(sizeof(pid_t) * pid_count);
    if (pids == NULL) {
        closedir(proc_dir);
        *count = 0;
        return NULL;
    }
    
    /* Second pass: collect PIDs */
    closedir(proc_dir);
    proc_dir = opendir(PROC_BASE_PATH);
    if (proc_dir == NULL) {
        free(pids);
        *count = 0;
        return NULL;
    }
    
    int idx = 0;
    
    while ((entry = readdir(proc_dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char* endptr;
            long pid_val = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && pid_val > 0) {
                pids[idx++] = (pid_t)pid_val;
            }
        }
    }
    
    closedir(proc_dir);
    *count = pid_count;
    return pids;
}

/*
 * read_proc_file - Read a file from /proc filesystem
 * @pid: Process ID (0 for system-wide /proc files)
 * @filename: Name of file to read (e.g., "status", "fd", "locks")
 * @return: Allocated string with file contents, or NULL on error
 * Description: Reads /proc/[PID]/filename or /proc/filename.
 *              Handles process termination gracefully. Caller must free result.
 *              Time complexity: O(file_size)
 * Error handling: Returns NULL on file open error, read error, or if process
 *                 terminated (ENOENT). Logs errors appropriately.
 */
char* read_proc_file(pid_t pid, const char* filename)
{
    if (filename == NULL) {
        error_log("read_proc_file: filename is NULL");
        return NULL;
    }
    
    /* Use utility function for safe reading */
    return read_proc_file_safe((int)pid, filename);
}

/*
 * parse_process_status - Parse /proc/[PID]/status file content
 * @content: Content of status file
 * @info: Output structure to fill
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Parses status file format and extracts process information.
 *              Handles missing fields gracefully.
 *              Time complexity: O(n) where n is file size
 * Error handling: Returns ERROR_INVALID_ARGUMENT if content is malformed
 */
int parse_process_status(const char* content, ProcessInfo* info)
{
    if (content == NULL || info == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    /* Initialize info structure */
    memset(info, 0, sizeof(ProcessInfo));
    info->fds = NULL;
    info->num_fds = 0;
    
    /* Parse line by line */
    char* content_copy = str_dup(content);
    if (content_copy == NULL) {
        return ERROR_OUT_OF_MEMORY;
    }
    
    char* line = content_copy;
    
    while (*line != '\0') {
        char* line_end = strchr(line, '\n');
        if (line_end != NULL) {
            *line_end = '\0';
        }
        
        /* Parse Name: field */
        if (str_starts_with(line, "Name:")) {
            char* name_start = line + 5; /* Skip "Name:" */
            str_trim(name_start);
            size_t name_len = strlen(name_start);
            if (name_len > 0 && name_len < MAX_PROCESS_NAME_LEN) {
                strncpy(info->name, name_start, MAX_PROCESS_NAME_LEN - 1);
                info->name[MAX_PROCESS_NAME_LEN - 1] = '\0';
            }
        }
        
        /* Parse State: field */
        else if (str_starts_with(line, "State:")) {
            char* state_start = line + 6; /* Skip "State:" */
            str_trim(state_start);
            if (*state_start != '\0') {
                info->state = *state_start;
            }
        }
        
        /* Parse PPid: field */
        else if (str_starts_with(line, "PPid:")) {
            char* ppid_start = line + 5; /* Skip "PPid:" */
            str_trim(ppid_start);
            info->ppid = (pid_t)strtol(ppid_start, NULL, 10);
        }
        
        /* Parse Uid: field (first value is real UID) */
        else if (str_starts_with(line, "Uid:")) {
            char* uid_start = line + 4; /* Skip "Uid:" */
            str_trim(uid_start);
            info->uid = (uid_t)strtol(uid_start, NULL, 10);
        }
        
        /* Parse Gid: field (first value is real GID) */
        else if (str_starts_with(line, "Gid:")) {
            char* gid_start = line + 4; /* Skip "Gid:" */
            str_trim(gid_start);
            info->gid = (gid_t)strtol(gid_start, NULL, 10);
        }
        
        /* Parse VmRSS: field (Resident Set Size in KB) */
        else if (str_starts_with(line, "VmRSS:")) {
            char* rss_start = line + 6; /* Skip "VmRSS:" */
            str_trim(rss_start);
            /* Extract number before "kB" */
            info->vm_rss = (unsigned long)strtoul(rss_start, NULL, 10);
        }
        
        /* Parse Threads: field */
        else if (str_starts_with(line, "Threads:")) {
            char* threads_start = line + 8; /* Skip "Threads:" */
            str_trim(threads_start);
            info->num_threads = (int)strtol(threads_start, NULL, 10);
        }
        
        if (line_end != NULL) {
            line = line_end + 1;
        } else {
            break;
        }
    }
    
    free(content_copy);
    return SUCCESS;
}

/*
 * get_process_info - Get detailed information about a specific process
 * @pid: Process ID to query
 * @info: Output structure to fill with process information
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Reads /proc/[PID]/status and parses process information.
 *              Fills ProcessInfo structure with parsed data.
 *              Time complexity: O(1) file read
 * Error handling: Returns ERROR_FILE_NOT_FOUND if process doesn't exist,
 *                 ERROR_PERMISSION_DENIED if access denied,
 *                 ERROR_OUT_OF_MEMORY on allocation failure
 */
int get_process_info(pid_t pid, ProcessInfo* info)
{
    if (info == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    if (pid <= 0) {
        return ERROR_INVALID_PROCESS_ID;
    }
    
    info->pid = pid;
    
    /* Check cache first */
    CacheEntry* cache_entry = get_cache_entry(pid);
    char* status_content = NULL;
    
    if (cache_entry != NULL && cache_entry->status_content != NULL) {
        status_content = cache_entry->status_content;
    } else {
        /* Read from file */
        status_content = read_proc_file(pid, PROC_STATUS_FILE);
        if (status_content == NULL) {
            if (errno == ENOENT) {
                return ERROR_FILE_NOT_FOUND;
            } else if (errno == EACCES) {
                return ERROR_PERMISSION_DENIED;
            } else {
                return ERROR_SYSTEM_CALL_FAILED;
            }
        }
        
        /* Store in cache */
        if (cache_entry != NULL) {
            cache_entry->status_content = status_content;
            cache_entry->timestamp = time(NULL);
        }
    }
    
    /* Parse status content */
    int result = parse_process_status(status_content, info);
    info->pid = pid; /* Ensure PID is set */
    
    /* Don't free cached content */
    if (cache_entry == NULL || cache_entry->status_content != status_content) {
        free(status_content);
    }
    
    if (result != SUCCESS) {
        return result;
    }
    
    /* Get open file descriptors */
    int fd_result = get_open_files(pid, &info->fds, &info->num_fds);
    if (fd_result != SUCCESS) {
        /* Non-fatal error, continue without FDs */
        debug_log("Failed to get open files for PID %d", pid);
        info->fds = NULL;
        info->num_fds = 0;
    }
    
    return SUCCESS;
}

/*
 * get_open_files - Get list of open file descriptors for a process
 * @pid: Process ID
 * @fds: Output array of file descriptor numbers
 * @count: Output parameter for number of FDs found
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Reads /proc/[PID]/fd directory to enumerate open FDs.
 *              Allocates array for FDs. Caller must free fds array.
 *              Time complexity: O(f) where f is number of FDs
 * Error handling: Returns error codes for access issues
 */
int get_open_files(pid_t pid, int** fds, int* count)
{
    if (fds == NULL || count == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *fds = NULL;
    *count = 0;
    
    char fd_dir_path[MAX_PATH_LEN];
    int result = snprintf(fd_dir_path, sizeof(fd_dir_path), 
                          "%s/%d/%s", PROC_BASE_PATH, (int)pid, PROC_FD_DIR);
    if (result < 0 || (size_t)result >= sizeof(fd_dir_path)) {
        return ERROR_BUFFER_OVERFLOW;
    }
    
    DIR* fd_dir = opendir(fd_dir_path);
    if (fd_dir == NULL) {
        if (errno == ENOENT) {
            return ERROR_FILE_NOT_FOUND;
        } else if (errno == EACCES) {
            return ERROR_PERMISSION_DENIED;
        } else {
            return ERROR_SYSTEM_CALL_FAILED;
        }
    }
    
    /* Count file descriptors */
    struct dirent* entry;
    int fd_count = 0;
    
    while ((entry = readdir(fd_dir)) != NULL) {
        if (entry->d_type == DT_LNK || isdigit((unsigned char)entry->d_name[0])) {
            char* endptr;
            long fd_val = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && fd_val >= 0) {
                fd_count++;
            }
        }
    }
    
    if (fd_count == 0) {
        closedir(fd_dir);
        return SUCCESS;
    }
    
    /* Allocate array */
    *fds = (int*)safe_malloc(sizeof(int) * fd_count);
    if (*fds == NULL) {
        closedir(fd_dir);
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Collect file descriptors */
    closedir(fd_dir);
    fd_dir = opendir(fd_dir_path);
    if (fd_dir == NULL) {
        free(*fds);
        *fds = NULL;
        *count = 0;
        return ERROR_SYSTEM_CALL_FAILED;
    }
    
    int idx = 0;
    
    while ((entry = readdir(fd_dir)) != NULL) {
        if (entry->d_type == DT_LNK || isdigit((unsigned char)entry->d_name[0])) {
            char* endptr;
            long fd_val = strtol(entry->d_name, &endptr, 10);
            if (*endptr == '\0' && fd_val >= 0) {
                (*fds)[idx++] = (int)fd_val;
            }
        }
    }
    
    closedir(fd_dir);
    *count = fd_count;
    return SUCCESS;
}

/*
 * get_file_locks - Get file locks held by a process
 * @pid: Process ID
 * @locks: Output array of FileLockInfo structures
 * @count: Output parameter for number of locks found
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Parses /proc/[PID]/locks to extract file lock information.
 *              Allocates array for locks. Caller must free locks array.
 *              Time complexity: O(l) where l is number of locks
 * Error handling: Returns error codes for file access or parse errors
 */
int get_file_locks(pid_t pid, FileLockInfo** locks, int* count)
{
    if (locks == NULL || count == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    *locks = NULL;
    *count = 0;
    
    char* locks_content = read_proc_file(pid, PROC_LOCKS_FILE);
    if (locks_content == NULL) {
        if (errno == ENOENT) {
            return ERROR_FILE_NOT_FOUND;
        } else if (errno == EACCES) {
            return ERROR_PERMISSION_DENIED;
        } else {
            return ERROR_SYSTEM_CALL_FAILED;
        }
    }
    
    /* Count locks (each line is a lock) */
    int lock_count = 0;
    const char* p = locks_content;
    while (*p != '\0') {
        if (*p == '\n') {
            lock_count++;
        }
        p++;
    }
    if (*locks_content != '\0' && locks_content[strlen(locks_content) - 1] != '\n') {
        lock_count++;
    }
    
    if (lock_count == 0) {
        free(locks_content);
        return SUCCESS;
    }
    
    /* Allocate array */
    *locks = (FileLockInfo*)safe_malloc(sizeof(FileLockInfo) * lock_count);
    if (*locks == NULL) {
        free(locks_content);
        return ERROR_OUT_OF_MEMORY;
    }
    
    /* Parse locks */
    char* line = locks_content;
    int idx = 0;
    
    while (*line != '\0' && idx < lock_count) {
        char* line_end = strchr(line, '\n');
        if (line_end != NULL) {
            *line_end = '\0';
        }
        
        /* Parse lock line format: "1: FLOCK ADVISORY WRITE 1234 00:12:345678 0 EOF" */
        FileLockInfo* lock = &(*locks)[idx];
        memset(lock, 0, sizeof(FileLockInfo));
        
        /* Basic parsing - extract lock ID and type */
        if (sscanf(line, "%d: %cLOCK", &lock->lock_id, &lock->lock_type) >= 2) {
            /* Try to extract PID from line */
            char* pid_str = strstr(line, " ");
            if (pid_str != NULL) {
                pid_str = strchr(pid_str + 1, ' ');
                if (pid_str != NULL) {
                    pid_str = strchr(pid_str + 1, ' ');
                    if (pid_str != NULL) {
                        pid_str = strchr(pid_str + 1, ' ');
                        if (pid_str != NULL) {
                            lock->pid = (int)strtol(pid_str + 1, NULL, 10);
                        }
                    }
                }
            }
            idx++;
        }
        
        if (line_end != NULL) {
            line = line_end + 1;
        } else {
            break;
        }
    }
    
    free(locks_content);
    *count = idx;
    return SUCCESS;
}

/*
 * get_process_resources - Get resource allocation information for a process
 * @pid: Process ID to query
 * @res_info: Output structure to fill with resource information
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Analyzes /proc/[PID]/fd and /proc/[PID]/locks to determine
 *              which resources the process holds and is waiting for.
 *              Builds ProcessResourceInfo structure.
 *              Time complexity: O(f + l) where f=num FDs, l=num locks
 * Error handling: Returns error codes for file access issues,
 *                 partial success if some data unavailable
 */
int get_process_resources(pid_t pid, ProcessResourceInfo* res_info)
{
    if (res_info == NULL) {
        return ERROR_INVALID_ARGUMENT;
    }
    
    memset(res_info, 0, sizeof(ProcessResourceInfo));
    res_info->pid = (int)pid;
    
    /* Get file locks (held resources) */
    FileLockInfo* locks = NULL;
    int lock_count = 0;
    int lock_result = get_file_locks(pid, &locks, &lock_count);
    
    if (lock_result == SUCCESS && lock_count > 0) {
        /* Allocate arrays for held resources */
        res_info->held_resources = (int*)safe_malloc(sizeof(int) * lock_count);
        res_info->held_files = (char**)safe_malloc(sizeof(char*) * lock_count);
        
        if (res_info->held_resources != NULL && res_info->held_files != NULL) {
            res_info->num_held = lock_count;
            res_info->num_held_files = lock_count;
            
            for (int i = 0; i < lock_count; i++) {
                /* Use lock ID as resource ID */
                res_info->held_resources[i] = locks[i].lock_id;
                /* Copy file path if available */
                if (strlen(locks[i].file_path) > 0) {
                    res_info->held_files[i] = str_dup(locks[i].file_path);
                } else {
                    char lock_id_str[32];
                    snprintf(lock_id_str, sizeof(lock_id_str), "lock_%d", locks[i].lock_id);
                    res_info->held_files[i] = str_dup(lock_id_str);
                }
            }
        }
        
        free_file_lock_info(locks, lock_count);
    }
    
    /* For now, we don't detect waiting resources directly from /proc
     * This would require more complex analysis of process state */
    res_info->num_waiting = 0;
    res_info->waiting_resources = NULL;
    res_info->num_waiting_files = 0;
    res_info->waiting_files = NULL;
    
    return SUCCESS;
}

/* =============================================================================
 * CLEANUP FUNCTIONS
 * =============================================================================
 */

/*
 * free_process_info - Free memory allocated in ProcessInfo structure
 * @info: ProcessInfo structure to clean up
 * @return: None
 * Description: Frees all dynamically allocated fields in ProcessInfo.
 *              Safe to call with partially initialized structures.
 * Error handling: Handles NULL pointers gracefully
 */
void free_process_info(ProcessInfo* info)
{
    if (info == NULL) {
        return;
    }
    
    if (info->fds != NULL) {
        free(info->fds);
        info->fds = NULL;
    }
    
    info->num_fds = 0;
}

/*
 * free_process_list - Free array of process IDs
 * @pids: Array of PIDs to free
 * @return: None
 * Description: Frees array allocated by get_all_processes().
 * Error handling: Handles NULL pointer safely
 */
void free_process_list(pid_t* pids)
{
    if (pids != NULL) {
        free(pids);
    }
}

/*
 * free_process_resource_info - Free memory in ProcessResourceInfo structure
 * @res_info: ProcessResourceInfo structure to clean up
 * @return: None
 * Description: Frees all dynamically allocated arrays in ProcessResourceInfo.
 *              Safe to call with partially initialized structures.
 * Error handling: Handles NULL pointers gracefully
 */
void free_process_resource_info(ProcessResourceInfo* res_info)
{
    if (res_info == NULL) {
        return;
    }
    
    if (res_info->held_resources != NULL) {
        free(res_info->held_resources);
        res_info->held_resources = NULL;
    }
    
    if (res_info->waiting_resources != NULL) {
        free(res_info->waiting_resources);
        res_info->waiting_resources = NULL;
    }
    
    if (res_info->held_files != NULL) {
        for (int i = 0; i < res_info->num_held_files; i++) {
            if (res_info->held_files[i] != NULL) {
                free(res_info->held_files[i]);
            }
        }
        free(res_info->held_files);
        res_info->held_files = NULL;
    }
    
    if (res_info->waiting_files != NULL) {
        for (int i = 0; i < res_info->num_waiting_files; i++) {
            if (res_info->waiting_files[i] != NULL) {
                free(res_info->waiting_files[i]);
            }
        }
        free(res_info->waiting_files);
        res_info->waiting_files = NULL;
    }
    
    res_info->num_held = 0;
    res_info->num_waiting = 0;
    res_info->num_held_files = 0;
    res_info->num_waiting_files = 0;
}

/*
 * free_file_lock_info - Free array of FileLockInfo structures
 * @locks: Array of FileLockInfo to free
 * @count: Number of locks in array
 * @return: None
 * Description: Frees array allocated by get_file_locks().
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void free_file_lock_info(FileLockInfo* locks, int count)
{
    if (locks != NULL && count > 0) {
        free(locks);
    }
}

/*
 * is_process_alive - Check if a process is still running
 * @pid: Process ID to check
 * @return: 1 if process exists, 0 otherwise
 * Description: Checks /proc/[PID] existence to determine if process is alive.
 *              Time complexity: O(1) system call
 * Error handling: Returns 0 on any error (including permission denied)
 */
int is_process_alive(pid_t pid)
{
    if (pid <= 0) {
        return 0;
    }
    
    char path[MAX_PATH_LEN];
    int result = snprintf(path, sizeof(path), "%s/%d", PROC_BASE_PATH, (int)pid);
    if (result < 0 || (size_t)result >= sizeof(path)) {
        return 0;
    }
    
    return file_exists(path);
}

