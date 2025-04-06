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

    *used_memory = 0;
    *total_memory = 0;

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open /proc/meminfo\n");
        return;
    }

    char label[64], unit[32], line[256];
    long int value;
    long int mem_total = 0, mem_free = 0;

    while (fgets(line, sizeof(line), fp)) {
        if (sscanf(line, "%63[^:]: %ld %31s", label, &value, unit) == 3) {
            if (strcmp(label, "MemTotal") == 0) {
                mem_total = value;
            } else if (strcmp(label, "MemFree") == 0) {
                mem_free = value;
            }
        }

        if (mem_total > 0 && mem_free > 0) {
            break; // stop early once we have what we need
        }
    }

    fclose(fp);

    *total_memory = mem_total;
    *used_memory = mem_total - mem_free;
}

long int calculate_memory_utilization(long int used_memory) {
    return used_memory / 1024 / 1024; // Convert from KB to GB
}
