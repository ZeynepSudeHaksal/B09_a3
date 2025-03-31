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
