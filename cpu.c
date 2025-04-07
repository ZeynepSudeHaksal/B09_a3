#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

int read_cpu_times(CpuTimes *times) {
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
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

double calculate_cpu_utilization(CpuTimes *prev, CpuTimes *curr) {
    long int total_prev = prev->user + prev->nice + prev->system +
                          prev->idle + prev->iowait + prev->irq + prev->softirq;

    long int total_curr = curr->user + curr->nice + curr->system +
                          curr->idle + curr->iowait + curr->irq + curr->softirq;

    long int total_diff = total_curr - total_prev;
    long int idle_diff = curr->idle - prev->idle;

    if (total_diff == 0) return 0.0;

    return 100.0 * (total_diff - idle_diff) / total_diff;
}
