#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

// Include the header files for each module
#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

volatile sig_atomic_t quit = 0;

void handle_sigint(int sig) {
    quit = 1;
}

void handle_sigtstp(int sig) {
    printf("\nCtrl-Z pressed, but ignored.\n");
}

int main(int argc, char *argv[]) {
    int samp = 20;
    int delay = 500000;
    int mem = 0, cp = 0, core = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (isdigit(*argv[i])) {
            if (i == 1) {
                samp = atoi(argv[i]);
            } else if (i == 2) {
                delay = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "--memory") == 0) {
            mem = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            cp = 1;
        } else if (strcmp(argv[i], "--cores") == 0) {
            core = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0) {
            samp = atoi(argv[i] + 10);
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
            delay = atoi(argv[i] + 9);
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            return 1;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    // Setup pipes
    int pipes[3][2];
    for (int i = 0; i < 3; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Failed to create pipes");
            return 1;
        }
    }

    // Fork processes
    for (int i = 0; i < 3; i++) {
        pid_t pid = fork();
        if (pid == -1) {
            perror("Failed to fork");
            return 1;
        } else if (pid == 0) { // Child process
            close(pipes[i][0]); // Close read end in child

            if ((i == 0 && mem) || (i == 1 && cp) || (i == 2 && core)) {
                printf("Child process %d started.\n", i);
            }

            if (i == 0 && mem) { // Memory process
                long int used_memory, total_memory;
                get_memory_usage(&used_memory, &total_memory);
                if (write(pipes[i][1], &used_memory, sizeof(used_memory)) != sizeof(used_memory) ||
                    write(pipes[i][1], &total_memory, sizeof(total_memory)) != sizeof(total_memory)) {
                    perror("Failed to write memory data to pipe");
                    exit(1);
                }
            } else if (i == 1 && cp) { // CPU process
                long int total, idle;
                read_cpu_times(&total, &idle);
                if (write(pipes[i][1], &total, sizeof(total)) != sizeof(total) ||
                    write(pipes[i][1], &idle, sizeof(idle)) != sizeof(idle)) {
                    perror("Failed to write CPU data to pipe");
                    exit(1);
                }
            } else if (i == 2 && core) { // Cores process
                int num_cores, max_freq;
                get_core_info(&num_cores, &max_freq);
                if (write(pipes[i][1], &num_cores, sizeof(num_cores)) != sizeof(num_cores) ||
                    write(pipes[i][1], &max_freq, sizeof(max_freq)) != sizeof(max_freq)) {
                    perror("Failed to write core data to pipe");
                    exit(1);
                }
            }
            close(pipes[i][1]); // Close write end
            exit(0);
        } else {
            close(pipes[i][1]); // Close write end in parent
        }
    }

    // Read from pipes in parent
    int num_cores, max_freq;
    long int used_memory, total_memory, total, idle;
    ssize_t bytes_read;
    if (mem) {
        bytes_read = read(pipes[0][0], &used_memory, sizeof(used_memory));
        bytes_read += read(pipes[0][0], &total_memory, sizeof(total_memory));
        if (bytes_read < 2 * sizeof(long int)) {
            perror("Failed to read full memory data from pipe");
        } else {
            printf("Memory data read: used %ld kB, total %ld kB.\n", used_memory, total_memory);
        }
    }
    if (cp) {
        bytes_read = read(pipes[1][0], &total, sizeof(total));
        bytes_read += read(pipes[1][0], &idle, sizeof(idle));
        if (bytes_read < 2 * sizeof(long int)) {
            perror("Failed to read full CPU data from pipe");
        } else {
            printf("CPU data read: total %ld, idle %ld.\n", total, idle);
        }
    }
    if (core) {
        bytes_read = read(pipes[2][0], &num_cores, sizeof(num_cores));
        bytes_read += read(pipes[2][0], &max_freq, sizeof(max_freq));
        if (bytes_read < 2 * sizeof(int)) {
            perror("Failed to read full core data from pipe");
        } else {
            printf("Core data read: number of cores %d, max frequency %d MHz.\n", num_cores, max_freq);
        }
    }

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    // Check if the signal flag has been set to terminate
    if (quit) {
        printf("Termination signal received, exiting program...\n");
        return 0;
    }

    return 0;
}
