#ifndef CPU_H
#define CPU_H

#include <stdio.h>

void read_cpu_times(long int *total, long int *idle);
double calculate_cpu_utilization();

#endif
