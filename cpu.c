#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "cpu.h"

// Static variables to preserve previous CPU times between calls
static long int prev_total = 0;
static long int prev_idle = 0;

// Reads CPU times and calculates total and idle
int get_cpu_times(long int *total, long int *idle) {
    if (!total || !idle) {
        fprintf(stderr, "Null pointers provided to get_cpu_times\n");
        return -1;
    }

    FILE *fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return -1;
    }

    long int user, nice, system, idle_val, iowait, irq, softirq;

    if (fscanf(fp, "cpu %ld %ld %ld %ld %ld %ld %ld", &user, &nice, &system, &idle_val, &iowait, &irq, &softirq) != 7) {
        perror("Failed to read CPU times");
        fclose(fp);
        return -1;
    }

    fclose(fp);

    *idle = idle_val + iowait;
    *total = user + nice + system + idle_val + iowait + irq + softirq;

    return 0;
}

// Calculates CPU usage percentage since last call
double calculate_cpu_utilization() {
    long int curr_total = 0, curr_idle = 0;

    if (get_cpu_times(&curr_total, &curr_idle) != 0) {
        return -1.0;
    }

    long int total_delta = curr_total - prev_total;
    long int idle_delta = curr_idle - prev_idle;

    prev_total = curr_total;
    prev_idle = curr_idle;

    if (total_delta == 0) return 0.0;

    return 100.0 * (total_delta - idle_delta) / total_delta;
}
