#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>

#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    int samples = 20;             // Default number of samples
    int tdelay = 500000;          // Default delay in microseconds (0.5 seconds)
    int mem_flag = 0;             // Monitor memory?
    int cpu_flag = 0;             // Monitor CPU?
    int cores_flag = 0;           // Monitor cores?

    // Command-line parsing
    for (int i = 1; i < argc; i++) {
        if (i == 1 && isdigit(*argv[i])) samples = atoi(argv[i]);
        else if (i == 2 && isdigit(*argv[i])) tdelay = atoi(argv[i]) * 1000;
        else if (strcmp(argv[i], "--memory") == 0) mem_flag = 1;
        else if (strcmp(argv[i], "--cpu") == 0) cpu_flag = 1;
        else if (strcmp(argv[i], "--cores") == 0) cores_flag = 1;
    }

    if (!mem_flag && !cpu_flag && !cores_flag) {
        mem_flag = cpu_flag = cores_flag = 1;  // default to all
    }

    // Pipes and PIDs
    int mem_pipe[2], cpu_pipe[2], core_pipe[2];
    pid_t mem_pid, cpu_pid, core_pid;

    // MEMORY pipe
    if (mem_flag && pipe(mem_pipe) == -1) {
        perror("Memory pipe failed");
        exit(EXIT_FAILURE);
    }

    // CPU pipe
    if (cpu_flag && pipe(cpu_pipe) == -1) {
        perror("CPU pipe failed");
        exit(EXIT_FAILURE);
    }

    // CORES pipe
    if (cores_flag && pipe(core_pipe) == -1) {
        perror("Cores pipe failed");
        exit(EXIT_FAILURE);
    }

    // Memory child
    if (mem_flag) {
        mem_pid = fork();
        if (mem_pid == 0) {
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

    // CPU child
    if (cpu_flag) {
        cpu_pid = fork();
        if (cpu_pid == 0) {
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

    // Cores child
    int num_cores = 0, max_freq = 0;
    if (cores_flag) {
        core_pid = fork();
        if (core_pid == 0) {
            close(core_pipe[READ_END]);
            get_core_info(&num_cores, &max_freq);
            write(core_pipe[WRITE_END], &num_cores, sizeof(int));
            write(core_pipe[WRITE_END], &max_freq, sizeof(int));
            close(core_pipe[WRITE_END]);
            exit(0);  // only needs to write once
        } else {
            close(core_pipe[WRITE_END]);
        }
    }

    // Arrays and variables for storing data
    long int memory_usage, overall_memory = 0;
    double cpu_usage;
    long int memo_util_arr[samples];
    double cpu_value_arr[samples];

    // Read core info once
    if (cores_flag) {
        read(core_pipe[READ_END], &num_cores, sizeof(int));
        read(core_pipe[READ_END], &max_freq, sizeof(int));
        close(core_pipe[READ_END]);
    }

    // Sampling loop
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

        graph(samples, tdelay, mem_flag, cpu_flag, cores_flag,
              num_cores, memo_util_arr, overall_memory, max_freq,
              cpu_value_arr, i);

        usleep(tdelay);
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
    if (cores_flag) {
        kill(core_pid, SIGKILL);  // technically not needed as it exits immediately
    }

    while (wait(NULL) > 0);  // Wait for all children to finish
    return 0;
}
