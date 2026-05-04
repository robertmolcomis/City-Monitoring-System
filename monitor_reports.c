#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>

// Volatile flag to control our while loop safely from within the signal handler
volatile sig_atomic_t keep_running = 1;

// Signal handler for SIGINT (Ctrl+C or kill -2)
void handle_sigint(int sig) {
    (void)sig; // Suppress unused parameter warning
    // Write is async-signal-safe, unlike printf
    const char msg[] = "\nReceived SIGINT. Shutting down monitor...\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
    keep_running = 0;
}

// Signal handler for SIGUSR1 (Called by city_manager on --add)
void handle_sigusr1(int sig) {
    (void)sig;
    const char msg[] = "Monitor alert: A new report has been added to a district!\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main() {
    // 1. Setup sigaction for SIGINT
    struct sigaction sa_int;
    sa_int.sa_handler = handle_sigint;
    sigemptyset(&sa_int.sa_mask);
    sa_int.sa_flags = 0; 
    if (sigaction(SIGINT, &sa_int, NULL) == -1) {
        perror("Failed to setup SIGINT");
        return EXIT_FAILURE;
    }

    // 2. Setup sigaction for SIGUSR1
    struct sigaction sa_usr1;
    sa_usr1.sa_handler = handle_sigusr1;
    sigemptyset(&sa_usr1.sa_mask);
    sa_usr1.sa_flags = SA_RESTART; // Restart paused system calls like pause() if interrupted
    if (sigaction(SIGUSR1, &sa_usr1, NULL) == -1) {
        perror("Failed to setup SIGUSR1");
        return EXIT_FAILURE;
    }

    // 3. Create or overwrite hidden .monitor_pid file
    FILE *pid_file = fopen(".monitor_pid", "w");
    if (!pid_file) {
        perror("Failed to create .monitor_pid file");
        return EXIT_FAILURE;
    }
    
    fprintf(pid_file, "%d\n", getpid());
    fclose(pid_file);
    
    printf("Monitor Process Started [PID: %d]\n", getpid());
    printf("Waiting for signals in the background...\n");

    // 4. Stay alive until keep_running is flipped by SIGINT
    while (keep_running) {
        pause(); // Suspends execution until any signal is caught
    }

    // 5. Cleanup the hidden file upon exiting
    unlink(".monitor_pid");
    
    return EXIT_SUCCESS;
}
