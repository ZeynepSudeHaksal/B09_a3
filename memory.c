
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "memory.h"

void get_memory_usage(long int *used_memory, long int *total_memory) {
    FILE *fp = fopen("/proc/meminfo", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open /proc/meminfo\n");
        return;
    }
    
    char line[256];
    char label[256];
    long int value;
    while (fgets(line, sizeof(line), fp) != NULL) {
        sscanf(line, "%s %ld kB", label, &value);
        if (strcmp(label, "MemTotal:") == 0) {
            *total_memory = value;
        } else if (strcmp(label, "MemAvailable:") == 0) {
            *used_memory = *total_memory - value;
        }
    }
    fclose(fp);
}
