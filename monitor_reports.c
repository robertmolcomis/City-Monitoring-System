#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>

volatile sig_atomic_t keep_running = 1;

// Signal handler for SIGINT
void handle_sigint(int sig) {
    (void)sig; 
    // Prefixed with [END] so hub_mon knows the process is terminating
    const char msg[] = "[END] Received SIGINT. Shutting down monitor gracefully...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
}

// Signal handler for SIGUSR1
void handle_sigusr1(int sig) {
    (void)sig;
    // Prefixed with [ALERT] for new report notifications
    const char msg[] = "[ALERT] A new report has been added to a district!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main() {
    // --- PHASE 3 ADDITION: Check if monitor is already running ---
    FILE *pid_check = fopen(".monitor_pid", "r");
    if (pid_check) {
        pid_t existing_pid;
        if (fscanf(pid_check, "%d", &existing_pid) == 1) {
            // Print formatted error to stdout (which hub_mon will read via pipe)
            printf("[ERROR] Monitor already running with PID %d\n", existing_pid);
            fflush(stdout);
            fclose(pid_check);
            return EXIT_FAILURE; // Terminate early
        }
        fclose(pid_check);
    }
    // -------------------------------------------------------------

    // Setup sigaction for SIGINT
    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; 
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("[ERROR] Failed to setup SIGINT");
        return EXIT_FAILURE;
    }

    // Setup sigaction for SIGUSR1
    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART; 
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("[ERROR] Failed to setup SIGUSR1");
        return EXIT_FAILURE;
    }

    // Create hidden .monitor_pid file
    FILE *pid_file = fopen(".monitor_pid", "w");
    if (!pid_file) {
        printf("[ERROR] Failed to create .monitor_pid file\n");
        fflush(stdout);
        return EXIT_FAILURE;
    }
    
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);
    
    // Output formatted startup message
    printf("[INFO] Monitor Process Started [PID: %d]\n", getpid());
    printf("[INFO] Waiting for signals in the background...\n");
    fflush(stdout); // Crucial: forces the output into the pipe immediately

    // Sleep loop waiting for signals
    while (keep_running) {
        pause(); 
    }

    // Cleanup upon exiting
    unlink(".monitor_pid");
    
    return EXIT_SUCCESS;
}
