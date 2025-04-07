#ifndef CPU_H
#define CPU_H

#include <stdio.h>

typedef struct CpuTimes {
    long int user;
    long int nice;
    long int system;
    long int idle;
    long int iowait;
    long int irq;
    long int softirq;
} CpuTimes;

int read_cpu_times(CpuTimes *times);
double calculate_cpu_utilization();

#endif
