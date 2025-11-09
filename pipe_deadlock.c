#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipe1[2], pipe2[2];
    char buf;
    
    printf("========================================\n");
    printf("  REAL PIPE DEADLOCK SIMULATION\n");
    printf("========================================\n");
    printf("Process A: READ pipe2 (block), then WRITE pipe1\n");
    printf("Process B: READ pipe1 (block), then WRITE pipe2\n");
    printf("Both will block forever waiting for each other.\n");
    printf("Press Ctrl+C to stop.\n");
    printf("========================================\n\n");
    
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1) {
        perror("pipe");
        exit(1);
    }
    
    pid_t pid = fork();
    
    if (pid < 0) {
        perror("fork");
        exit(1);
    }
    
    if (pid == 0) {
        // Child process (Process B)
        printf("[Process B - PID %d] Started\n", getpid());
        
        close(pipe1[1]); // Close write end of pipe1
        close(pipe2[0]); // Close read end of pipe2
        
        printf("[Process B - PID %d] Trying to READ from pipe1... (WILL BLOCK FOREVER)\n", getpid());
        read(pipe1[0], &buf, 1);  // BLOCK - no data in pipe1!
        
        printf("[Process B - PID %d] Writing to pipe2 (never reaches)\n", getpid());
        write(pipe2[1], "B", 1);
        
        printf("[Process B - PID %d] Done (never reaches)\n", getpid());
        exit(0);
    } else {
        // Parent process (Process A)
        printf("[Process A - PID %d] Started\n", getpid());
        
        close(pipe1[0]); // Close read end of pipe1
        close(pipe2[1]); // Close write end of pipe2
        
        sleep(1); // Give child time to start
        
        printf("[Process A - PID %d] Trying to READ from pipe2... (WILL BLOCK FOREVER)\n", getpid());
        read(pipe2[0], &buf, 1);  // BLOCK - no data in pipe2!
        
        printf("[Process A - PID %d] Writing to pipe1 (never reaches)\n", getpid());
        write(pipe1[1], "A", 1);
        
        printf("[Process A - PID %d] Done (never reaches)\n", getpid());
        wait(NULL);
    }
    
    return 0;
}

