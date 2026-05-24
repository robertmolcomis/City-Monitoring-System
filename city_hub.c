#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/types.h>

#define MAX_INPUT 256

// Handles the "calculate_scores" command
void do_calculate_scores(char *args) {
    char *districts[20];
    int num_districts = 0;
    
    // Parse the district arguments separated by spaces
    char *token = strtok(args, " \n");
    while (token && num_districts < 20) {
        districts[num_districts++] = token;
        token = strtok(NULL, " \n");
    }

    if (num_districts == 0) {
        printf("Usage: calculate_scores <district1> <district2> ...\n");
        return;
    }

    printf("\n=== COMBINED WORKLOAD REPORT ===\n");
    
    for (int i = 0; i < num_districts; i++) {
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe failed");
            continue;
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            continue;
        }

        if (pid == 0) {
            // Child Process: Redirect standard output to the write-end of the pipe
            close(pipefd[0]); // Close unused read-end
            dup2(pipefd[1], STDOUT_FILENO); // Overwrite stdout with pipe
            close(pipefd[1]); // Close original write-end descriptor

            execl("./scorer", "scorer", districts[i], NULL);
            
            // If execl fails:
            perror("Failed to execute scorer");
            exit(EXIT_FAILURE);
        } else {
            // Parent Process (city_hub): Read from the pipe
            close(pipefd[1]); // Close unused write-end
            
            char buffer[256];
            FILE *pipe_read = fdopen(pipefd[0], "r");
            
            while (fgets(buffer, sizeof(buffer), pipe_read)) {
                printf("%s", buffer);
            }
            
            fclose(pipe_read);
            waitpid(pid, NULL, 0); // Wait for this specific scorer to finish
        }
    }
    printf("================================\n\n");
}

// Handles the "start_monitor" command
void do_start_monitor() {
    pid_t hub_mon_pid = fork();
    
    if (hub_mon_pid < 0) {
        perror("Fork failed");
        return;
    }

    if (hub_mon_pid == 0) {
        // --- Inside hub_mon child process ---
        int pipefd[2];
        if (pipe(pipefd) == -1) {
            perror("Pipe failed in hub_mon");
            exit(EXIT_FAILURE);
        }

        pid_t monitor_pid = fork();
        
        if (monitor_pid == 0) {
            // --- Inside monitor_reports child process ---
            close(pipefd[0]);
            dup2(pipefd[1], STDOUT_FILENO);
            close(pipefd[1]);
            
            execl("./monitor_reports", "monitor_reports", NULL);
            perror("Failed to execute monitor_reports");
            exit(EXIT_FAILURE);
        }
        
        // --- Back in hub_mon process ---
        close(pipefd[1]);
        char buffer[256];
        FILE *pipe_read = fdopen(pipefd[0], "r");
        
        // Continuously read the monitor's piped output line by line
        while (fgets(buffer, sizeof(buffer), pipe_read)) {
            printf("\r[HUB INTERCEPT] %s", buffer);
            
            // Check the prefixed tags we added in Iteration 1 to detect termination
            if (strstr(buffer, "[END]") || strstr(buffer, "[ERROR]")) {
                printf("\n>>> SYSTEM ALERT: The background monitor process has safely terminated. <<<\n");
                // Print prompt again to keep UI clean
                printf("city_hub> "); 
                fflush(stdout);
            }
        }
        
        fclose(pipe_read);
        waitpid(monitor_pid, NULL, 0);
        exit(EXIT_SUCCESS);
    }
    
    // Parent process (city_hub) returns immediately to the interactive prompt
    printf("Monitor started in background (hub_mon PID: %d).\n", hub_mon_pid);
}

int main() {
    char input[MAX_INPUT];
    
    printf("\n=== Welcome to City Hub ===\n");
    printf("Available commands:\n");
    printf(" - start_monitor\n");
    printf(" - calculate_scores <district1> <district2> ...\n");
    printf(" - exit\n\n");

    while (1) {
        printf("city_hub> ");
        if (!fgets(input, sizeof(input), stdin)) {
            break; // Handle EOF (Ctrl+D)
        }

        // Strip newline
        input[strcspn(input, "\n")] = '\0';
        
        if (strlen(input) == 0) continue;

        char *cmd = strtok(input, " ");
        char *args = strtok(NULL, ""); // Get the rest of the string

        if (strcmp(cmd, "exit") == 0 || strcmp(cmd, "quit") == 0) {
            break;
        } else if (strcmp(cmd, "start_monitor") == 0) {
            do_start_monitor();
        } else if (strcmp(cmd, "calculate_scores") == 0) {
            do_calculate_scores(args);
        } else {
            printf("Unknown command: %s\n", cmd);
        }
    }

    printf("Exiting City Hub. Goodbye!\n");
    return EXIT_SUCCESS;
}
