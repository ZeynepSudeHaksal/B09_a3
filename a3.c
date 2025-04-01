#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    int samples = 20;
    int tdelay = 500000;
    int mem_flag = 0;
    int cpu_flag = 0;
    int cores_flag = 0;

    // Parse optional arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--memory") == 0) {
            mem_flag = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            cpu_flag = 1;
        } else if (strcmp(argv[i], "--cores") == 0) {
            cores_flag = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0) {
            samples = atoi(argv[i] + 10);
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
            tdelay = atoi(argv[i] + 9);
        }
    }

    // Default to all if no specific flag is provided
    if (mem_flag == 0 && cpu_flag == 0 && cores_flag == 0) {
        mem_flag = cpu_flag = cores_flag = 1;
    }

    // Arrays to store utilization values for plotting
    long int *memory_utilizations = mem_flag ? malloc(samples * sizeof(long int)) : NULL;
    double *cpu_utilizations = cpu_flag ? malloc(samples * sizeof(double)) : NULL;

    int num_cores, max_freq;
    if (cores_flag) {
        get_core_info(&num_cores, &max_freq); // Assuming core info doesn't change, fetch once
    }

    pid_t pid;
    int mem_pipe[2], cpu_pipe[2];

    // Create pipes for memory and CPU data if required
    if (mem_flag) pipe(mem_pipe);
    if (cpu_flag) pipe(cpu_pipe);

    // Fork for memory data collection
    if (mem_flag) {
        pid = fork();
        if (pid == 0) { // Child process for memory
            close(mem_pipe[READ_END]);
            long int used_memory, total_memory;
            for (int i = 0; i < samples; i++) {
                get_memory_usage(&used_memory, &total_memory);
                long int mem_util = calculate_memory_utilization(used_memory);
                write(mem_pipe[WRITE_END], &mem_util, sizeof(long int));
                usleep(tdelay);
            }
            close(mem_pipe[WRITE_END]);
            exit(0);
        } else {
            close(mem_pipe[WRITE_END]);
        }
    }

    // Fork for CPU data collection
    if (cpu_flag) {
        pid = fork();
        if (pid == 0) { // Child process for CPU
            close(cpu_pipe[READ_END]);
            for (int i = 0; i < samples; i++) {
                double cpu_util = calculate_cpu_utilization();
                write(cpu_pipe[WRITE_END], &cpu_util, sizeof(double));
                usleep(tdelay);
            }
            close(cpu_pipe[WRITE_END]);
            exit(0);
        } else {
            close(cpu_pipe[WRITE_END]);
        }
    }

    // Read and graph data in the main process
    for (int i = 0; i < samples; i++) {
        if (mem_flag) {
            read(mem_pipe[READ_END], &memory_utilizations[i], sizeof(long int));
        }
        if (cpu_flag) {
            read(cpu_pipe[READ_END], &cpu_utilizations[i], sizeof(double));
        }
        // Call the graph function to update the display with each new set of data
        graph(samples, tdelay, mem_flag, cpu_flag, cores_flag, num_cores, memory_utilizations, 0 /*total memory not dynamically updated*/, max_freq, cpu_utilizations, i);
    }

    // Close pipes
    if (mem_flag) close(mem_pipe[READ_END]);
    if (cpu_flag) close(cpu_pipe[READ_END]);

    // Free dynamically allocated memory
    if (mem_flag) free(memory_utilizations);
    if (cpu_flag) free(cpu_utilizations);

    return 0;
}
