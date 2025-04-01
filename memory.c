#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "memory.h"

void get_memory_usage(long int *used_memory, long int *total_memory) {
    if (!used_memory || !total_memory) {
        fprintf(stderr, "Null pointers provided to get_memory_usage\n");
        return;
    }

    *used_memory = 0; // Initialize output variables
    *total_memory = 0;

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open /proc/meminfo\n");
        return;
    }

    char line[256], label[256], unit[32];
    long int value, mem_free = 0, buffers = 0, cached = 0, sreclaimable = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "%255s %ld %31s", label, &value, unit);
        if (strcmp(label, "MemTotal:") == 0) {
            *total_memory = value;
        } else if (strcmp(label, "MemFree:") == 0) {
            mem_free = value;
        } else if (strcmp(label, "Buffers:") == 0) {
            buffers = value;
        } else if (strcmp(label, "Cached:") == 0) {
            cached = value;
        } else if (strcmp(label, "SReclaimable:") == 0) {
            sreclaimable = value;
        }
    }
    fclose(fp);

    // Calculate used memory as total minus free and reclaimable buffers and cache
    *used_memory = *total_memory - mem_free - buffers - cached - sreclaimable;
}

long int calculate_memory_utilization(long int used_memory) {
    return used_memory / 1024 / 1024; // Convert from KB to GB
}
