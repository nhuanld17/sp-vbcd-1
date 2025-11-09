#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/file.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <signal.h>

#define FILE1 "/tmp/deadlock_lock1.tmp"
#define FILE2 "/tmp/deadlock_lock2.tmp"

void cleanup_handler(int sig) {
    printf("\n[CLEANUP] Received signal %d, cleaning up...\n", sig);
    unlink(FILE1);
    unlink(FILE2);
    exit(0);
}

void process_a() {
    printf("[Process A - PID %d] Started\n", getpid());
    
    // Create and lock FILE1
    int fd1 = open(FILE1, O_CREAT | O_RDWR, 0666);
    if (fd1 < 0) {
        perror("open FILE1");
        exit(1);
    }
    
    printf("[Process A - PID %d] Locking %s...\n", getpid(), FILE1);
    flock(fd1, LOCK_EX);
    printf("[Process A - PID %d] ✓ Locked %s\n", getpid(), FILE1);
    
    printf("[Process A - PID %d] Sleeping 2 seconds...\n", getpid());
    sleep(2);
    
    // Try to lock FILE2 (will deadlock)
    printf("[Process A - PID %d] Trying to lock %s...\n", getpid(), FILE2);
    int fd2 = open(FILE2, O_CREAT | O_RDWR, 0666);
    if (fd2 < 0) {
        perror("open FILE2");
        exit(1);
    }
    
    printf("[Process A - PID %d] Waiting for lock on %s... (BLOCKED)\n", getpid(), FILE2);
    flock(fd2, LOCK_EX);  // DEADLOCK HERE!
    
    printf("[Process A - PID %d] ✓ Locked %s (will never print)\n", getpid(), FILE2);
    
    close(fd1);
    close(fd2);
}

void process_b() {
    printf("[Process B - PID %d] Started\n", getpid());
    
    // Create and lock FILE2
    int fd2 = open(FILE2, O_CREAT | O_RDWR, 0666);
    if (fd2 < 0) {
        perror("open FILE2");
        exit(1);
    }
    
    printf("[Process B - PID %d] Locking %s...\n", getpid(), FILE2);
    flock(fd2, LOCK_EX);
    printf("[Process B - PID %d] ✓ Locked %s\n", getpid(), FILE2);
    
    printf("[Process B - PID %d] Sleeping 2 seconds...\n", getpid());
    sleep(2);
    
    // Try to lock FILE1 (will deadlock)
    printf("[Process B - PID %d] Trying to lock %s...\n", getpid(), FILE1);
    int fd1 = open(FILE1, O_CREAT | O_RDWR, 0666);
    if (fd1 < 0) {
        perror("open FILE1");
        exit(1);
    }
    
    printf("[Process B - PID %d] Waiting for lock on %s... (BLOCKED)\n", getpid(), FILE1);
    flock(fd1, LOCK_EX);  // DEADLOCK HERE!
    
    printf("[Process B - PID %d] ✓ Locked %s (will never print)\n", getpid(), FILE1);
    
    close(fd1);
    close(fd2);
}

int main() {
    printf("========================================\n");
    printf("  PROCESS DEADLOCK SIMULATION\n");
    printf("========================================\n");
    printf("This program creates 2 processes that will deadlock.\n");
    printf("Process A: locks FILE1, waits for FILE2\n");
    printf("Process B: locks FILE2, waits for FILE1\n");
    printf("Press Ctrl+C to stop.\n");
    printf("========================================\n\n");
    
    // Setup signal handler
    signal(SIGINT, cleanup_handler);
    signal(SIGTERM, cleanup_handler);
    
    // Create lock files
    system("touch /tmp/deadlock_lock1.tmp");
    system("touch /tmp/deadlock_lock2.tmp");
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(1);
    } else if (pid == 0) {
        // Child process - Process B
        process_b();
        exit(0);
    } else {
        // Parent process - Process A
        process_a();
        
        // Wait for child (will never reach here)
        wait(NULL);
    }
    
    // Cleanup (will never reach here)
    unlink(FILE1);
    unlink(FILE2);
    
    return 0;
}

