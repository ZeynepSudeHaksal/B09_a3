#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>
#include <errno.h>

#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

volatile sig_atomic_t quit = 0;

void handle_sigint(int sig) {
    quit = 1;
    printf("Signal interrupt received. Exiting gracefully...\n");
}

void handle_sigtstp(int sig) {
    printf("\nCtrl-Z pressed, but ignored.\n");
}

int main(int argc, char *argv[]) {
    int samp = 20;
    int delay = 500000;
    int mem = 0, cp = 0, core = 0;
    long int total_memory = 0;
    int num_cores = 0, max_freq = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (isdigit(*argv[i])) {
            if (i == 1) samp = atoi(argv[i]);
            else if (i == 2) delay = atoi(argv[i]);
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
            exit(1);
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    long int *memory_usage_array = malloc(samp * sizeof(long int));
    double *cpu_usage_array = malloc(samp * sizeof(double));
    if (memory_usage_array == NULL || cpu_usage_array == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }

    int pipes[3][2]; // Correct pipe array declaration

    for (int i = 0; i < samp && !quit; i++) {
        for (int j = 0; j < 3; j++) {
            if (pipe(pipes[j]) == -1) {
                perror("Failed to create pipe");
                exit(1);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("Failed to fork");
                exit(1);
            } else if (pid == 0) { // Child process
                close(pipes[j][0]); // Close read end in child
                if (j == 0 && mem) {
                    //long int temp_memory = calculate_memory_utilization();
                    long int temp_memory = 7;
                    printf("Debug: Child %d - Memory Utilization Calculated: %ld\n", j, temp_memory);
                    fflush(stdout);
                    write(pipes[j][1], &temp_memory, sizeof(temp_memory));
                } else if (j == 1 && cp) {
                    //double temp_cpu = calculate_cpu_utilization();
                    double temp_cpu = 3.50;
                    printf("Debug: Child %d - CPU Utilization Calculated: %.2f%%\n", j, temp_cpu);
                    fflush(stdout);

                    write(pipes[j][1], &temp_cpu, sizeof(temp_cpu));
                } else if (j == 2 && core) {
                    get_core_info(&num_cores, &max_freq);
                    printf("Debug: Child %d - Cores: %d, Max Frequency: %d\n", j, num_cores, max_freq);
                    fflush(stdout);
                    write(pipes[j][1], &num_cores, sizeof(num_cores));
                    write(pipes[j][1], &max_freq, sizeof(max_freq));
                }
                close(pipes[j][1]);
                exit(0);
            } else {
                close(pipes[j][1]); // Close write end in parent
            }
        }

        // Parent process reads data
        if (mem) {
            ssize_t bytesRead = read(pipes[0][0], &memory_usage_array[i], sizeof(long int));
            printf("Debug: Parent - Memory Usage Read: %ld (Bytes Read: %zd)\n", memory_usage_array[i], bytesRead);
        }
        if (cp) {
            ssize_t bytesRead = read(pipes[1][0], &cpu_usage_array[i], sizeof(double));
            printf("Debug: Parent - CPU Usage Read: %.2f%% (Bytes Read: %zd)\n", cpu_usage_array[i], bytesRead);
        }
        if (core) {
            ssize_t bytesRead = read(pipes[2][0], &num_cores, sizeof(int));
            printf("Debug: Parent - Cores Read: %d (Bytes Read: %zd)\n", num_cores, bytesRead);
            bytesRead = read(pipes[2][0], &max_freq, sizeof(int));
            printf("Debug: Parent - Max Frequency Read: %d (Bytes Read: %zd)\n", max_freq, bytesRead);
        }

        // Close all pipe ends in parent
        for (int j = 0; j < 3; j++) {
            close(pipes[j][0]);
        }

        // Wait for all child processes to finish
        while (wait(NULL) > 0);

        // Graph the results for the current sample
        if (mem || cp) {
            graph(samp, delay, mem, cp, core, num_cores, memory_usage_array, total_memory, max_freq, cpu_usage_array, i);
        }
        if (core) {
            draw_cores(num_cores);
        }

        usleep(delay);  // Sleep for 'delay' microseconds
    }

    free(memory_usage_array);
    free(cpu_usage_array);

    if (quit) {
        printf("Termination signal received, exiting program...\n");
        return 0;
    }

    return 0;
}
