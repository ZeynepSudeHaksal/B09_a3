#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>

#include "memory.h"
#include "cpu.h"
#include "graph.h"

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    int samples = 20;  // Default number of samples
    int tdelay = 500000;  // Default delay in microseconds (0.5 seconds)
    int mem_flag = 0;  // Monitor memory?
    int cpu_flag = 0;  // Monitor CPU?
    int cores_flag = 0;  // Monitor cores? Not implemented in detailed monitoring here

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--samples") == 0 && i + 1 < argc) {
            samples = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--tdelay") == 0 && i + 1 < argc) {
            tdelay = atoi(argv[++i]) * 1000;  // Convert ms to us
        } else if (strcmp(argv[i], "--memory") == 0) {
            mem_flag = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            cpu_flag = 1;
        } else if (strcmp(argv[i], "--cores") == 0) {
            cores_flag = 1;
        }
    }

    if (!mem_flag && !cpu_flag && !cores_flag) {
        mem_flag = cpu_flag = cores_flag = 1; // Default to monitor all if no specific flags are given
    }

    int mem_pipe[2], cpu_pipe[2];
    pid_t mem_pid, cpu_pid;

    // Create pipes
    if (mem_flag && pipe(mem_pipe) == -1) {
        perror("Failed to create memory pipe");
        exit(EXIT_FAILURE);
    }
    if (cpu_flag && pipe(cpu_pipe) == -1) {
        perror("Failed to create CPU pipe");
        exit(EXIT_FAILURE);
    }

    // Fork memory monitoring process
    if (mem_flag) {
        mem_pid = fork();
        if (mem_pid == 0) {  // Child process for memory monitoring
            close(mem_pipe[READ_END]);
            long int used_memory, total_memory;

            while (1) {
                get_memory_usage(&used_memory, &total_memory);
                write(mem_pipe[WRITE_END], &used_memory, sizeof(long int));
                write(mem_pipe[WRITE_END], &total_memory, sizeof(long int));
                usleep(tdelay);
            }
            exit(0);
        } else {
            close(mem_pipe[WRITE_END]);
        }
    }

    // Fork CPU monitoring process
    if (cpu_flag) {
        cpu_pid = fork();
        if (cpu_pid == 0) {  // Child process for CPU monitoring
            close(cpu_pipe[READ_END]);
            double cpu_utilization;

            while (1) {
                cpu_utilization = calculate_cpu_utilization();
                write(cpu_pipe[WRITE_END], &cpu_utilization, sizeof(double));
                usleep(tdelay);
            }
            exit(0);
        } else {
            close(cpu_pipe[WRITE_END]);
        }
    }

    long int memory_usage, overall_memory = 0;
    double cpu_usage;
    long int memo_util_arr[samples];
    double cpu_value_arr[samples];

    // Main loop to read data from pipes and graph dynamically
    for (int i = 0; i < samples; i++) {
        if (mem_flag) {
            if (read(mem_pipe[READ_END], &memory_usage, sizeof(long int)) > 0 &&
                read(mem_pipe[READ_END], &overall_memory, sizeof(long int)) > 0) {
                memo_util_arr[i] = memory_usage;
            }
        }
        if (cpu_flag) {
            if (read(cpu_pipe[READ_END], &cpu_usage, sizeof(double)) > 0) {
                cpu_value_arr[i] = cpu_usage;
            }
        }

        graph(samples, tdelay, mem_flag, cpu_flag, cores_flag, 0, memo_util_arr, overall_memory, 0, cpu_value_arr, i);
        usleep(tdelay); // Update graph at intervals
    }

    // Cleanup
    if (mem_flag) {
        close(mem_pipe[READ_END]);
        kill(mem_pid, SIGKILL);
    }
    if (cpu_flag) {
        close(cpu_pipe[READ_END]);
        kill(cpu_pid, SIGKILL);
    }

    while (wait(NULL) > 0); // Wait for all child processes to exit

    return 0;
}
