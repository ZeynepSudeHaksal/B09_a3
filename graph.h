#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>

void graph(int samp, int delay, int mem, int cp, int core, int numofcores, long int* memo_util_arr, long int overall_value, int maxfreq, double* cpu_value_arr, int j);
void draw_cores(int cores);

#endif
