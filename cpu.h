#ifndef CPU_H
#define CPU_H

#include <stdio.h>

int get_cpu_times(long int *total, long int *idle);
double calculate_cpu_utilization();

#endif
