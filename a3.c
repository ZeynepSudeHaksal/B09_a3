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
}

void handle_sigtstp(int sig) {
    printf("\nCtrl-Z pressed, but ignored.\n");
}

int main(int argc, char *argv[]) {
    int samp = 20; // Number of samples
    int delay = 500000; // Delay in microseconds between samples
    int mem = 0, cp = 0, core = 0;
    long int total_memory, used_memory, total, idle;
    int num_cores, max_freq;

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

    // Setup data arrays for graphing
    long int* memory_usage_array = malloc(samp * sizeof(long int));
    double* cpu_usage_array = malloc(samp * sizeof(double));
    if (memory_usage_array == NULL || cpu_usage_array == NULL) {
        printf("Memory allocation failed\n");
        exit(1);
    }

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

            if (i == 0 && mem) { // Memory process
                get_memory_usage(&used_memory, &total_memory);
                write(pipes[i][1], &used_memory, sizeof(used_memory));
                write(pipes[i][1], &total_memory, sizeof(total_memory));
            } else if (i == 1 && cp) { // CPU process
                read_cpu_times(&total, &idle);
                write(pipes[i][1], &total, sizeof(total));
                write(pipes[i][1], &idle, sizeof(idle));
            } else if (i == 2 && core) { // Cores process
                get_core_info(&num_cores, &max_freq);
                write(pipes[i][1], &num_cores, sizeof(num_cores));
                write(pipes[i][1], &max_freq, sizeof(max_freq));
            }
            close(pipes[i][1]); // Close write end
            exit(0);
        } else {
            close(pipes[i][1]); // Close write end in parent
        }
    }

    // Read from pipes in parent
    for (int i = 0; i < 3; i++) {
        if (i == 0 && mem) {
            read(pipes[i][0], &used_memory, sizeof(used_memory));
            read(pipes[i][0], &total_memory, sizeof(total_memory));
        } else if (i == 1 && cp) {
            read(pipes[i][0], &total, sizeof(total));
            read(pipes[i][0], &idle, sizeof(idle));
        } else if (i == 2 && core) {
            read(pipes[i][0], &num_cores, sizeof(num_cores));
            read(pipes[i][0], &max_freq, sizeof(max_freq));
        }
        close(pipes[i][0]);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
    }

    // Graphing data
    if (mem || cp) {
        for (int j = 0; j < samp; j++) {
            memory_usage_array[j] = used_memory; // Example to fill data
            cpu_usage_array[j] = (double)(total - idle) / total * 100.0;
        }
        graph(samp, delay, mem, cp, core, num_cores, memory_usage_array, total_memory, max_freq, cpu_usage_array, samp - 1);
    }
    if (core) {
        draw_cores(num_cores);
    }

    // Free allocated memory
    free(memory_usage_array);
    free(cpu_usage_array);

    // Handle termination signal
    if (quit) {
        printf("Termination signal received, exiting program...\n");
        return 0;
    }

    return 0;
}
