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
    long int value;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (sscanf(line, "%255s %ld %31s", label, &value, unit) == 3) {
            if (strcmp(label, "MemTotal:") == 0) {
                *total_memory = value;
            } else if (strcmp(label, "MemAvailable:") == 0) {
                *used_memory = *total_memory - value;
            }
        }
    }
    fclose(fp);
}

long int calculate_memory_utilization() {
    long int used_memory, total_memory;
    get_memory_usage(&used_memory, &total_memory);

    // Check for possible errors or invalid reads
    if (total_memory == 0 || used_memory == 0) {
        fprintf(stderr, "Memory data not correctly fetched\n");
        return -1;  // Error condition or invalid calculation
    }

    // Calculate the memory utilization (difference between total and used)
    long int memory_utilization = (used_memory) / 1024/ 1024;

    return memory_utilization;
}
