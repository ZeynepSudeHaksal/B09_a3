#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "graph.h"

//function for graphing to the terminal.
void graph(int sample, int delay, int mem, int cp, long int* mem_arr, long int overall_value, double* cpu_arr, int t)
    ///_|> descry: generates a graph of memory and CPU usage 
    ///_|> sample: total number of samples
    ///_|> delay: delay between samples in microseconds
    ///_|> mem: memory flag
    ///_|> cp: cpu flag 
    ///_|> mem_arr: array of memory usage values, type: long int*
    ///_|> overall_value: total system memory in kB, type: long int
    ///_|> cpu_arr: array of CPU usage values, type: double*
    ///_|> t: current sample index, type: int
    ///_|> returning: this function does not return anything
{
    printf("\033[2J\n");
    printf("\033[H\n");

    printf("Nbr of samples: %d --- every %d microSecs (%.3f secs)\n", sample, delay, (float)delay/1e6);

    printf("\n");

    if(mem)
    {
        //MEMORY
        printf("v Memory %.2f GB\n", mem_arr[t] / 1e6); 

        for(int n = 12; n > 0; n--)
        {
            if (n==12)
            {
                printf("%d GB |",(int)(overall_value/1e6)); 
            }
            else
            {
                printf("      |");
            }
            for(int i = 0; i < t; i++)
            {
                    if (ceil(mem_arr[i]*12.0/(int)(overall_value)) == n) 
                    {
                        printf("#");
                    }
                    else
                    {
                        printf(" ");
                    }
            }
            printf("\n");
        }
        
        printf("0 GB  ");
        for(int k = 0; k < sample+1; k++) 
        {
            printf("_");
        }
        printf("\n");
        printf("\n");
    }

    if((cp))
    {
        //CPU
        printf("v CPU %.2f %%\n",cpu_arr[t]);
        for(int n = 10; n > 0; n--)
        {
            if (n==10)
            {
                printf("100 %% |");
            }
            else {printf("      |");}
            for(int i = 0; i < t; i++)
            {
                    if (ceil(cpu_arr[i]*10/100) == n) 
                    {
                        printf(":");
                    }
                    else
                    {
                        printf(" ");
                    }
            }
            printf("\n");
        }
        printf("0 %%   ");
        for(int k = 0; k < sample+1; k++)
        {
            printf("_");
        }
        printf("\n");
        printf("\n");
    }

}   

void draw_cores(int core, int cores, double maxfreq) {
    ///_|> descry: draws a representation of CPU cores 
    ///_|> core: cores flag 
    ///_|> cores: number of cores to draw
    ///_|> max_freq:  max frequency in MHz
    ///_|> returning: this function does not return anything
    if (!core){
        return;  // only draw if the core flag is active
    }

    printf("Number of Cores : %d @ %.2f GHz\n",cores ,(double)(maxfreq/1e6));

    int rows = cores / 4;
    if (cores % 4 != 0) {
        rows++;
    }

    for (int i = 0; i < rows; i++) {
        // Top border of each core
        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("+----+  ");
            else
                printf("        ");
        }
        printf("\n");

        // Core content row
        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("|    |  ");
            else
                printf("        ");
        }
        printf("\n");

        // Bottom border of each core
        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("+----+  ");
            else
                printf("        ");
        }
        printf("\n");
    }
}

