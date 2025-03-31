#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

void get_core_info(int *num_cores, int *max_freq) {
    FILE *fp = fopen("/proc/cpuinfo", "r");
    if (!fp) {
        fprintf(stderr, "Failed to open /proc/cpuinfo\n");
        return;
    }

    char line[256];
    int count = 0;
    while (fgets(line, sizeof(line), fp) != NULL) {
        if (strstr(line, "processor\t:")) {
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

    fscanf(freq_fp, "%d", max_freq);
    fclose(freq_fp);
    fclose(fp);
}