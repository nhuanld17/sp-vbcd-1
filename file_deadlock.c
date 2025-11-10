#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

#define FILE1 "/tmp/deadlock_file1.lock"
#define FILE2 "/tmp/deadlock_file2.lock"

void cleanup_files() {
    unlink(FILE1);
    unlink(FILE2);
}

void signal_handler(int sig) {
    printf("\n[SIGNAL] Caught signal %d, cleaning up...\n", sig);
    cleanup_files();
    exit(0);
}

void process_a() {
    int fd1, fd2;
    
    printf("[Process A - PID %d] Started\n", getpid());
    
    // Open and lock FILE1
    fd1 = open(FILE1, O_CREAT | O_RDWR, 0666);
    if (fd1 < 0) {
        perror("open FILE1");
        exit(1);
    }
    
    printf("[Process A - PID %d] Locking %s...\n", getpid(), FILE1);
    if (flock(fd1, LOCK_EX) < 0) {
        perror("flock FILE1");
        exit(1);
    }
    printf("[Process A - PID %d] ✓ Locked %s\n", getpid(), FILE1);
    
    // Sleep to ensure process B gets FILE2 lock
    printf("[Process A - PID %d] Sleeping 2 seconds...\n", getpid());
    sleep(2);
    
    // Try to lock FILE2 (will deadlock)
    printf("[Process A - PID %d] Trying to lock %s...\n", getpid(), FILE2);
    fd2 = open(FILE2, O_CREAT | O_RDWR, 0666);
    if (fd2 < 0) {
        perror("open FILE2");
        exit(1);
    }
    
    printf("[Process A - PID %d] Waiting for lock on %s... (WILL BLOCK)\n", getpid(), FILE2);
    flock(fd2, LOCK_EX);  // DEADLOCK HERE!
    
    printf("[Process A - PID %d] ✓ Locked %s (never reaches)\n", getpid(), FILE2);
    
    // Cleanup (never reaches)
    flock(fd1, LOCK_UN);
    flock(fd2, LOCK_UN);
    close(fd1);
    close(fd2);
}

void process_b() {
    int fd1, fd2;
    
    printf("[Process B - PID %d] Started\n", getpid());
    
    // Open and lock FILE2
    fd2 = open(FILE2, O_CREAT | O_RDWR, 0666);
    if (fd2 < 0) {
        perror("open FILE2");
        exit(1);
    }
    
    printf("[Process B - PID %d] Locking %s...\n", getpid(), FILE2);
    if (flock(fd2, LOCK_EX) < 0) {
        perror("flock FILE2");
        exit(1);
    }
    printf("[Process B - PID %d] ✓ Locked %s\n", getpid(), FILE2);
    
    // Sleep to ensure process A gets FILE1 lock
    printf("[Process B - PID %d] Sleeping 2 seconds...\n", getpid());
    sleep(2);
    
    // Try to lock FILE1 (will deadlock)
    printf("[Process B - PID %d] Trying to lock %s...\n", getpid(), FILE1);
    fd1 = open(FILE1, O_CREAT | O_RDWR, 0666);
    if (fd1 < 0) {
        perror("open FILE1");
        exit(1);
    }
    
    printf("[Process B - PID %d] Waiting for lock on %s... (WILL BLOCK)\n", getpid(), FILE1);
    flock(fd1, LOCK_EX);  // DEADLOCK HERE!
    
    printf("[Process B - PID %d] ✓ Locked %s (never reaches)\n", getpid(), FILE1);
    
    // Cleanup (never reaches)
    flock(fd1, LOCK_UN);
    flock(fd2, LOCK_UN);
    close(fd1);
    close(fd2);
}

int main() {
    pid_t pid;
    
    printf("========================================\n");
    printf("  FILE LOCK DEADLOCK SIMULATION\n");
    printf("========================================\n");
    printf("This program creates 2 processes that will deadlock on file locks.\n");
    printf("Process A: locks FILE1, waits for FILE2\n");
    printf("Process B: locks FILE2, waits for FILE1\n");
    printf("Press Ctrl+C to stop.\n");
    printf("========================================\n\n");
    
    // Setup signal handler
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);
    
    // Create lock files
    FILE* f1 = fopen(FILE1, "w");
    FILE* f2 = fopen(FILE2, "w");
    if (f1) fclose(f1);
    if (f2) fclose(f2);
    
    // Fork child process
    pid = fork();
    
    if (pid < 0) {
        perror("fork");
        cleanup_files();
        exit(1);
    } else if (pid == 0) {
        // Child process - Process B
        process_b();
        exit(0);
    } else {
        // Parent process - Process A
        process_a();
        
        // Wait for child (never reaches here)
        wait(NULL);
    }
    
    // Cleanup (never reaches here)
    cleanup_files();
    
    return 0;
}

