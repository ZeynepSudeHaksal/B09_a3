#include <stdio.h>
#include <stdlib.h>
#include "cpu.h"

int read_cpu_times(CpuTimes *times) {
    ///_|> descry: reads current CPU time values from /proc/stat and fills the CpuTimes struct
    ///_|> times: pointer to a CpuTimes struct to store CPU info
    ///_|> returning: returns 0 on success, -1 on failure
    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return -1;
    }

    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld", &times->user, &times->nice, &times->system, &times->idle, &times->iowait, &times->irq, &times->softirq) != 7) {
        perror("Failed to read CPU times");
        fclose(fp);
        return -1;
    }

    fclose(fp);
    return 0;
}

double calculate_cpu_utilization(CpuTimes *prev, CpuTimes *curr) {
    ///_|> descry: calculates the CPU utilization percentage between two readings
    ///_|> prev: previous CPU times
    ///_|> curr: current CPU times
    ///_|> returning: CPU usage percentage between the two time samples
    long int total_prev = prev->user + prev->nice + prev->system + prev->idle + prev->iowait + prev->irq + prev->softirq;

    long int total_curr = curr->user + curr->nice + curr->system + curr->idle + curr->iowait + curr->irq + curr->softirq;

    long int total_diff = total_curr - total_prev;
    long int idle_diff = curr->idle - prev->idle;

    if (total_diff == 0){
        return 0.0;
    } 

    return 100.0 * (total_diff - idle_diff) / total_diff;
}
