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
int read_cpu_times(CpuTimes *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/stat");
        return -1;
    }
    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld",
               &times->user,
               &times->nice,
               &times->system,
               &times->idle,
               &times->iowait,
               &times->irq,
               &times->softirq) != 7) {
        perror("Failed to read CPU times");
        fclose(fp);
        return -1;
    }
    fclose(fp);
    return 0;
}

double calculate_cpu_utilization() {
    static CpuTimes prev = {0};  // static to keep value between calls
    CpuTimes curr;

    // Fetch the current CPU times
    if (read_cpu_times(&curr) != 0) {
        return -1;  // return an error code or handle error appropriately
    }

    // Calculate total time since last check
    long int total_prev = prev.user + prev.nice + prev.system + prev.idle +
                          prev.iowait + prev.irq + prev.softirq;
    long int total_curr = curr.user + curr.nice + curr.system + curr.idle +
                          curr.iowait + curr.irq + curr.softirq;
    long int total_delta = total_curr - total_prev;
    long int idle_delta = curr.idle - prev.idle;

    // Save current times for the next calculation
    prev = curr;

    if (total_delta == 0) {
        return 0.0;  // prevent division by zero
    }

    // Calculate the percentage of CPU utilization
    return 100.0 * (total_delta - idle_delta) / total_delta;
}
