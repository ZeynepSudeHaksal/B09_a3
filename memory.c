#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "memory.h"

// Function to read current memory info
int read_mem_info(MemInfo *info) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (fp == NULL) {
        perror("Failed to open /proc/meminfo");
        return -1;
    }

    char line[256], label[64], unit[32];
    long int value;
    int found_total = 0, found_free = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%63[^:]: %ld %31s", label, &value, unit) == 3) {
            if (strcmp(label, "MemTotal") == 0) {
                info->total = value;
                found_total = 1;
            } else if (strcmp(label, "MemFree") == 0) {
                info->free = value;
                found_free = 1;
            }
        }

        if (found_total && found_free) break;  // stop early if both values found
    }

    fclose(fp);

    if (!found_total || !found_free) {
        fprintf(stderr, "Failed to parse memory info\n");
        return -1;
    }

    return 0;
}

// Function to calculate memory utilization as a percentage
double calculate_memory_utilization() {
    MemInfo mem;

    if (read_mem_info(&mem) != 0) {
        return -1;  // error reading memory
    }

    if (mem.total == 0) {
        return 0.0;  // prevent division by zero
    }

    long int used = mem.total - mem.free;
    return 100.0 * used / mem.total;
}
