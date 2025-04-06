#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>

void graph(int samp, int delay, int mem, int cp, int core, long int* memo_util_arr, long int overall_value, double* cpu_value_arr, int j);
void draw_cores(int cores);

#endif
