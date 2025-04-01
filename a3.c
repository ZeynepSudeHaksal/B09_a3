#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "memory.h"
#include "cpu.h"
#include "graph.h"

#define READ_END 0
#define WRITE_END 1

int main(int argc, char *argv[]) {
    int samples = 20;
    int tdelay = 500000; // delay in microseconds
    int mem_flag = 1;    // Assuming we always want to monitor memory
    int cpu_flag = 1;    // Assuming we always want to monitor CPU

    int mem_pipe[2], cpu_pipe[2];
    pid_t mem_pid, cpu_pid;

    // Create pipes
    if (pipe(mem_pipe) == -1 || pipe(cpu_pipe) == -1) {
        perror("Failed to create pipes");
        return EXIT_FAILURE;
    }

    // Fork for memory data collection
    mem_pid = fork();
    if (mem_pid == 0) {  // Child process for memory
        close(mem_pipe[READ_END]);
        long int used_memory, total_memory;

        while (1) {
            get_memory_usage(&used_memory, &total_memory);
            write(mem_pipe[WRITE_END], &used_memory, sizeof(long int));
            usleep(tdelay);  // Collect data at intervals
        }
        close(mem_pipe[WRITE_END]);
        exit(0);
    }

    // Fork for CPU data collection
    cpu_pid = fork();
    if (cpu_pid == 0) {  // Child process for CPU
        close(cpu_pipe[READ_END]);
        double cpu_utilization;

        while (1) {
            cpu_utilization = calculate_cpu_utilization();
            write(cpu_pipe[WRITE_END], &cpu_utilization, sizeof(double));
            usleep(tdelay);  // Collect data at intervals
        }
        close(cpu_pipe[WRITE_END]);
        exit(0);
    }

    close(mem_pipe[WRITE_END]);
    close(cpu_pipe[WRITE_END]);

    long int memory_usage;
    double cpu_usage;
    long int overall_value = 1e9; // Example total memory for scaling
    int maxfreq = 0; // Not used in this simplified example
    long int memo_util_arr[samples];
    double cpu_value_arr[samples];

    // Main loop to read data from pipes and graph dynamically
    for (int i = 0; i < samples; i++) {
        if (read(mem_pipe[READ_END], &memory_usage, sizeof(long int)) > 0) {
            memo_util_arr[i] = memory_usage; // Store the value
        }
        if (read(cpu_pipe[READ_END], &cpu_usage, sizeof(double)) > 0) {
            cpu_value_arr[i] = cpu_usage; // Store the value
        }

        // Graph the results dynamically
        graph(i + 1, tdelay, 1, 1, 0, 0, memo_util_arr, overall_value, maxfreq, cpu_value_arr, i);
        usleep(tdelay); // Update graph at intervals
    }

    // Cleanup
    close(mem_pipe[READ_END]);
    close(cpu_pipe[READ_END]);
    kill(mem_pid, SIGKILL);
    kill(cpu_pid, SIGKILL);
    while (wait(NULL) > 0); // Wait for all child processes to exit

    return 0;
}
