#ifndef GRAPH_H
#define GRAPH_H

#include <stdio.h>

void graph(int sample, int delay, int mem, int cp, long int* mem_arr, long int overall_value, double* cpu_arr, int t);
void draw_cores(int core, int cores, double maxfreq);

#endif
