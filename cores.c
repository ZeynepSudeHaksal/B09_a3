#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "cores.h"

void get_core_info(int *num_cores, double *max_freq) {
    ///_|> descry: reads the number of CPU cores and their maximum frequency
    ///_|> num_cores: pointer to an integer to store the number of cores
    ///_|> max_freq: pointer to a double to store the max frequency in MHz
    ///_|> returning: this function does not return anything
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open /proc/cpuinfo\n");
        return;
    }

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strncmp(line, "processor", 9) == 0) {
            count++;
        }

    }
    *num_cores = count;

    FILE *freq_fp = fopen("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq", "r");
    if (!freq_fp) {
        fprintf(stderr, "Failed to open cpuinfo_max_freq\n");
        fclose(fp);
        return;
    }

    fscanf(freq_fp, "%lf", max_freq);
    fclose(freq_fp);
    fclose(fp);
}