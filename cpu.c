
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

void read_cpu_times(long int *total, long int *idle) {
    FILE* fp = fopen("/proc/stat", "r");
    if (!fp) {
        perror("Failed to open /proc/stat");
        return;
    }

    char buffer[256];
    fgets(buffer, sizeof(buffer), fp);
    long int user, nice, system, idle_time, iowait, irq, softirq, steal;
    sscanf(buffer, "cpu %ld %ld %ld %ld %ld %ld %ld %ld",
           &user, &nice, &system, &idle_time, &iowait, &irq, &softirq, &steal);
    *total = user + nice + system + idle_time + iowait + irq + softirq + steal;
    *idle = idle_time + iowait;
    
    fclose(fp);
}