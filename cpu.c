#include <stdio.h>
#include <stdlib.h>

typedef struct CpuTimes {
    long int user;
    long int nice;
    long int system;
    long int idle;
    long int iowait;
    long int irq;
    long int softirq;
} CpuTimes;

// Function to fetch the current CPU times
void read_cpu_times(CpuTimes *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/stat");
        exit(1);
    }
    fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld",
           &times->user,
           &times->nice,
           &times->system,
           &times->idle,
           &times->iowait,
           &times->irq,
           &times->softirq);
    fclose(fp);
}

double calculate_cpu_utilization() {
    static CpuTimes prev = {0};
    CpuTimes curr;

    // Fetch the current CPU times
    read_cpu_times(&curr);

    // Calculate total time since last check
    long int total_prev = prev.user + prev.nice + prev.system + prev.idle +
                          prev.iowait + prev.irq + prev.softirq;
    long int total_curr = curr.user + curr.nice + curr.system + curr.idle +
                          curr.iowait + curr.irq + curr.softirq;
    long int total_delta = total_curr - total_prev;

    // Calculate idle time since last check
    long int idle_delta = curr.idle - prev.idle;

    // Save current times for the next calculation
    prev = curr;

    if (total_delta == 0) {  // Prevent division by zero
        return 0.0;
    }

    // Calculate the percentage of CPU utilization
    return 100.0 * (total_delta - idle_delta) / total_delta;
}
