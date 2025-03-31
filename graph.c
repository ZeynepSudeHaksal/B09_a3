#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>

#include "graph.h"

//function for graphing to the terminal.
void graph(int samp, int delay, int mem, int cp, int core, int numofcores, long int* memo_util_arr, long int overall_value, int maxfreq, double* cpu_value_arr, int j)
{
    printf("\033[2J\n");
    printf("\033[H\n");

    printf("Nbr of samples: %d --- every %d microSecs (%.3f secs)\n", samp, delay, (float)delay/1e6);

    printf("\n");

    if(!(mem || cp || core) || (mem))//all 0 or memory 1
    {
        //MEMORY
        printf("v Memory %.2f GB\n", memo_util_arr[j] / 1e6); 

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
            for(int i = 0; i < j; i++)
            {
                    if (ceil(memo_util_arr[i]*12.0/(int)(overall_value)) == n) //how many bars does it fill?
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
        for(int o = 0; o < samp+1; o++) //+1 for the _ for the axis
        {
            printf("_");
        }
        printf("\n");
        printf("\n");
    }

    if(!(mem || cp || core) || (cp))
    {
        //CPU
        printf("v CPU %.2f %%\n",cpu_value_arr[j]);
        for(int n = 10; n > 0; n--)
        {
            if (n==10)
            {
                printf("100 %% |");
            }
            else {printf("      |");}
            for(int i = 0; i < j; i++)
            {
                    if (ceil(cpu_value_arr[i]*10/100) == n) //how many bars does it fill?
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
        for(int o = 0; o < samp+1; o++)
        {
            printf("_");
        }
        printf("\n");
        printf("\n");
    }

}   

void draw_cores(int cores) {
    int rows = cores / 4;
    if (cores % 4 != 0) rows++;

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("+----+  ");
            else
                printf("       ");
        }
        printf("\n");

        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("|    |  ");
            else
                printf("       ");
        }
        printf("\n");

        for (int j = 0; j < 4; j++) {
            if (i * 4 + j < cores)
                printf("+----+  ");
            else
                printf("       ");
        }
        printf("\n");
    }
}
