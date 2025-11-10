#ifndef PROCESS_MONITOR_H
#define PROCESS_MONITOR_H

/* =============================================================================
 * PROCESS_MONITOR.H - Process Monitoring Interface
 * =============================================================================
 * This header defines structures and functions for collecting process
 * information from the /proc filesystem on Linux systems.
 * =============================================================================
 */

#include <sys/types.h>
#include "config.h"

/* =============================================================================
 * DATA STRUCTURES
 * =============================================================================
 */

/*
 * ProcessInfo - Complete process information
 * Stores basic process details parsed from /proc/[PID]/status
 */
typedef struct {
    pid_t pid;                      /* Process ID */
    char name[MAX_PROCESS_NAME_LEN]; /* Process name (from Name: field) */
    char state;                     /* Process state: R, S, D, Z, T, W, X */
    pid_t ppid;                     /* Parent process ID */
    uid_t uid;                      /* User ID */
    gid_t gid;                      /* Group ID */
    unsigned long vm_rss;           /* Resident Set Size in KB */
    int num_threads;                /* Number of threads */
    int* fds;                       /* Array of open file descriptors */
    int num_fds;                    /* Number of file descriptors */
    char* wchan;                    /* Wait channel (what kernel function process is blocked on) */
} ProcessInfo;

/*
 * ProcessResourceInfo - Process resource allocation information
 * Tracks which resources a process holds and which it's waiting for
 */
typedef struct {
    int pid;                        /* Process ID */
    int* held_resources;            /* Array of resource IDs this process holds */
    int num_held;                   /* Number of held resources */
    int* waiting_resources;         /* Array of resource IDs this process waits for */
    int num_waiting;                /* Number of waiting resources */
    char** held_files;              /* Array of file paths held by process */
    int num_held_files;             /* Number of held files */
    char** waiting_files;           /* Array of file paths process is waiting for */
    int num_waiting_files;          /* Number of waiting files */
    char* wchan;                    /* Wait channel (kernel function process is blocked on) */
    int* waiting_on_pids;           /* Array of PIDs this process is waiting for (for pipe/lock deadlocks) */
    int num_waiting_on_pids;        /* Number of PIDs being waited on */
    unsigned long* pipe_inodes;     /* Array of pipe inodes this process has open */
    int num_pipe_inodes;            /* Number of pipe inodes */
    int* pipe_fds;                  /* Array of file descriptors corresponding to pipe_inodes */
    int is_blocked_on_pipe;         /* 1 if process is blocked waiting on pipe read/write */
    int is_blocked_on_lock;         /* 1 if process is blocked waiting on file lock */
} ProcessResourceInfo;

/*
 * FileLockInfo - Information about a file lock
 * Parsed from /proc/[PID]/locks or /proc/locks
 */
typedef struct {
    int lock_id;                    /* Lock ID */
    char lock_type;                 /* 'F' for FLOCK, 'P' for POSIX */
    int pid;                        /* Process ID holding the lock */
    char file_path[MAX_PATH_LEN];   /* Path to locked file */
    unsigned long start;            /* Start offset */
    unsigned long end;              /* End offset */
    unsigned long inode;            /* Inode number of locked file */
    int is_blocking;                /* 1 if this lock is blocking another process */
} FileLockInfo;

/*
 * PipeInfo - Information about a pipe
 * Extracted from /proc/[PID]/fd/ file descriptors
 */
typedef struct {
    unsigned long inode;            /* Pipe inode number */
    int fd;                         /* File descriptor number */
    pid_t pid;                      /* Process ID that has this pipe open */
    int is_read_end;                /* 1 if this is read end, 0 if write end */
    int is_blocked;                 /* 1 if process is blocked on this pipe */
} PipeInfo;

/* =============================================================================
 * FUNCTION PROTOTYPES
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
pid_t* get_all_processes(int* count);

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
int get_process_info(pid_t pid, ProcessInfo* info);

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
int get_process_resources(pid_t pid, ProcessResourceInfo* res_info);

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
char* read_proc_file(pid_t pid, const char* filename);

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
int parse_process_status(const char* content, ProcessInfo* info);

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
int get_open_files(pid_t pid, int** fds, int* count);

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
int get_file_locks(pid_t pid, FileLockInfo** locks, int* count);

/*
 * free_process_info - Free memory allocated in ProcessInfo structure
 * @info: ProcessInfo structure to clean up
 * @return: None
 * Description: Frees all dynamically allocated fields in ProcessInfo.
 *              Safe to call with partially initialized structures.
 * Error handling: Handles NULL pointers gracefully
 */
void free_process_info(ProcessInfo* info);

/*
 * free_process_list - Free array of process IDs
 * @pids: Array of PIDs to free
 * @return: None
 * Description: Frees array allocated by get_all_processes().
 * Error handling: Handles NULL pointer safely
 */
void free_process_list(pid_t* pids);

/*
 * free_process_resource_info - Free memory in ProcessResourceInfo structure
 * @res_info: ProcessResourceInfo structure to clean up
 * @return: None
 * Description: Frees all dynamically allocated arrays in ProcessResourceInfo.
 *              Safe to call with partially initialized structures.
 * Error handling: Handles NULL pointers gracefully
 */
void free_process_resource_info(ProcessResourceInfo* res_info);

/*
 * free_file_lock_info - Free array of FileLockInfo structures
 * @locks: Array of FileLockInfo to free
 * @count: Number of locks in array
 * @return: None
 * Description: Frees array allocated by get_file_locks().
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void free_file_lock_info(FileLockInfo* locks, int count);

/*
 * is_process_alive - Check if a process is still running
 * @pid: Process ID to check
 * @return: 1 if process exists, 0 otherwise
 * Description: Checks /proc/[PID] existence to determine if process is alive.
 *              Time complexity: O(1) system call
 * Error handling: Returns 0 on any error (including permission denied)
 */
int is_process_alive(pid_t pid);

/*
 * get_process_wchan - Get wait channel for a process
 * @pid: Process ID to query
 * @wchan: Output parameter for wait channel string (caller must free)
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Reads /proc/[PID]/wchan to determine what kernel function
 *              the process is blocked on. Returns empty string if not blocked.
 *              Time complexity: O(1) file read
 * Error handling: Returns error codes for file access issues
 */
int get_process_wchan(pid_t pid, char** wchan);

/*
 * parse_system_locks - Parse /proc/locks to get all file locks in system
 * @locks: Output array of FileLockInfo structures
 * @count: Output parameter for number of locks found
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Parses system-wide /proc/locks file to extract all file locks.
 *              Allocates array for locks. Caller must free locks array.
 *              Time complexity: O(l) where l is number of locks
 * Error handling: Returns error codes for file access or parse errors
 */
int parse_system_locks(FileLockInfo** locks, int* count);

/*
 * get_pipe_info_from_fd - Get pipe inode from file descriptor
 * @pid: Process ID
 * @fd: File descriptor number
 * @inode: Output parameter for pipe inode
 * @is_read_end: Output parameter (1 if read end, 0 if write end)
 * @return: SUCCESS (0) if FD is a pipe, negative error code otherwise
 * Description: Reads /proc/[PID]/fd/[FD] to determine if it's a pipe
 *              and extract its inode number. Determines read/write end.
 *              Time complexity: O(1) file read
 * Error handling: Returns error if FD is not a pipe or access denied
 */
int get_pipe_info_from_fd(pid_t pid, int fd, unsigned long* inode, int* is_read_end);

/*
 * get_file_path_from_fd - Get file path from file descriptor
 * @pid: Process ID
 * @fd: File descriptor number
 * @file_path: Output parameter for file path (must be MAX_PATH_LEN bytes)
 * @inode: Output parameter for file inode (optional, can be NULL)
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Reads /proc/[PID]/fd/[FD] symlink to get file path and inode.
 *              Extracts inode from /proc/[PID]/fdinfo/[FD] if needed.
 *              Time complexity: O(1) file read
 * Error handling: Returns error if FD access denied or not a regular file
 */
int get_file_path_from_fd(pid_t pid, int fd, char* file_path, unsigned long* inode);

/*
 * detect_pipe_dependencies - Detect pipe relationships between processes
 * @all_pipes: Output array of PipeInfo structures for all processes
 * @pipe_count: Output parameter for number of pipes found
 * @pids: Array of process IDs to analyze
 * @num_pids: Number of processes
 * @return: SUCCESS (0) on success, negative error code on failure
 * Description: Analyzes all processes to find pipe relationships.
 *              Matches pipe inodes between processes to detect dependencies.
 *              Allocates array for pipes. Caller must free pipes array.
 *              Time complexity: O(P * F) where P=processes, F=FDs per process
 * Error handling: Returns error codes for allocation or access issues
 */
int detect_pipe_dependencies(PipeInfo** all_pipes, int* pipe_count,
                             pid_t* pids, int num_pids);

/*
 * free_pipe_info - Free array of PipeInfo structures
 * @pipes: Array of PipeInfo to free
 * @count: Number of pipes in array
 * @return: None
 * Description: Frees array allocated by detect_pipe_dependencies().
 * Error handling: Handles NULL pointers and invalid counts safely
 */
void free_pipe_info(PipeInfo* pipes, int count);

#endif /* PROCESS_MONITOR_H */

