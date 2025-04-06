#ifndef MEMORY_H
#define MEMORY_H

typedef struct MemInfo {
    long int total;
    long int free;
} MemInfo;

// Reads total and free memory into a MemInfo struct
int read_mem_info(MemInfo *info);

// Optionally, calculate percentage of used memory (not required if unused)
double calculate_memory_utilization();

#endif // MEMORY_H
