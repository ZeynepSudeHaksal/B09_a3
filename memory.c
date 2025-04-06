#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "memory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void get_memory_usage(long int *used_memory, long int *total_memory) {
    if (!used_memory || !total_memory) {
        fprintf(stderr, "Null pointers provided to get_memory_usage\n");
        return;
    }

    *used_memory = 0;
    *total_memory = 0;

    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        perror("Failed to open /proc/meminfo");
        return;
    }

    char line[256];
    char label[64];
    long int value;
    int found_total = 0;
    int found_available = 0;
    long int available_mem = 0;

    while (fgets(line, sizeof(line), fp)) {
        // Scan label and value from line
        if (sscanf(line, "%s %ld", label, &value) != 2) continue;

        if (strcmp(label, "MemTotal:") == 0) {
            *total_memory = value;
            found_total = 1;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            available_mem = value;
            found_available = 1;
        }

        if (found_total && found_available) break;
    }

    fclose(fp);

    *used_memory = *total_memory - available_mem;
}


long int calculate_memory_utilization(long int used_memory) {
    return used_memory / 1024 / 1024; // Convert from KB to GB
}
