#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "cpu.h"

void read_cpu_times(long int *total, long int *idle) {
    if (!total || !idle) {
        fprintf(stderr, "Null pointer provided to read_cpu_times\n");
        return;
    }

    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return;
    }

    char buffer[256];
    if (fgets(buffer, sizeof(buffer), fp) == NULL) {
        fprintf(stderr, "Failed to read /proc/stat\n");
        fclose(fp);
        return;
    }

    long int user, nice, system, idle_time, iowait, irq, softirq, steal;
    int count = sscanf(buffer, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
                       &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);
    if (count != 8) {
        fprintf(stderr, "Failed to parse CPU times\n");
        fclose(fp);
        return;
    }

    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    *idle = idle_time + iowait;
    
    fclose(fp);
}
