#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <ctype.h>
#include <errno.h>


#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

// Flags to trigger shutdown
volatile sig_atomic_t should_quit = 0;
volatile sig_atomic_t sigint_triggered = 0;

// Handle Ctrl+C
void handle_sigint(int sig) {
   sigint_triggered = 1;
}

// Handle Ctrl+Z
void handle_sigtstp(int sig) {
    char msg[] = "This is ctrl z, continuing\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

}

int main(int argc, char *argv[]) {
    //signal handling
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    int samples = 20;
    int tdelay = 500000;
    int mem_flag = 0, cpu_flag = 0, cores_flag = 0;

    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (i == 1 && isdigit(*argv[i])) {
            samples = atoi(argv[i]);
        }
        else if (i == 2 && isdigit(*argv[i])) {
            tdelay = atoi(argv[i]);
        }
        else if (strcmp(argv[i], "--memory") == 0){
            mem_flag = 1;
        } 
        else if (strcmp(argv[i], "--cpu") == 0){
             cpu_flag = 1;
        }
        else if (strcmp(argv[i], "--cores") == 0) {
            cores_flag = 1;
        }
    }

    if (!mem_flag && !cpu_flag && !cores_flag) {
        mem_flag = cpu_flag = cores_flag = 1;
    }
    pid_t mem_pid = -1, cpu_pid = -1, core_pid = -1;
    int mem_pipe[2], cpu_pipe[2], core_pipe[2];

    // Memory pipe
    if (mem_flag && pipe(mem_pipe) == -1) {
        perror("Error creating memory pipe");
        exit(EXIT_FAILURE);
    }

    // CPU pipe
    if (cpu_flag && pipe(cpu_pipe) == -1) {
        perror("Error creating CPU pipe");
        exit(EXIT_FAILURE);
    }

    // Core pipe
    if (cores_flag && pipe(core_pipe) == -1) {
        perror("Error creating core pipe");
        exit(EXIT_FAILURE);
    }

    // Fork memory process
    if (mem_flag) {
        mem_pid = fork();
        if (mem_pid == -1) {
            perror("Error forking memory process");
            exit(EXIT_FAILURE);
        } 
        else if (mem_pid == 0) {
            close(mem_pipe[0]);
            long int used, total;
            while (1) {
                get_memory_usage(&used, &total);
                if (write(mem_pipe[1], &used, sizeof(long int)) == -1 ||
                    write(mem_pipe[1], &total, sizeof(long int)) == -1) {
                    perror("Error writing to memory pipe");
                    exit(EXIT_FAILURE);
                }
                usleep(tdelay);
            }
        } else {
            close(mem_pipe[1]);
        }
    }

    // Fork CPU process
    if (cpu_flag) {
        cpu_pid = fork();
        if (cpu_pid == -1) {
            perror("Error forking CPU process");
            exit(EXIT_FAILURE);
        } 
        else if (cpu_pid == 0) {
            close(cpu_pipe[0]);
            double cpu;
            while (1) {
                cpu = calculate_cpu_utilization();
                if (write(cpu_pipe[1], &cpu, sizeof(double)) == -1) {
                    perror("Error writing to CPU pipe");
                    exit(EXIT_FAILURE);
                }
                usleep(tdelay);
            }
        } else {
            close(cpu_pipe[1]);
        }
    }

    // Fork Cores process
    int num_cores = 0;
    double max_freq = 0;
    if (cores_flag) {
        core_pid = fork();
        if (core_pid == -1) {
            perror("Error forking cores process");
            exit(EXIT_FAILURE);
        } 
        else if (core_pid == 0) {
            close(core_pipe[0]);
            get_core_info(&num_cores, &max_freq);
            if (write(core_pipe[1], &num_cores, sizeof(int)) == -1 ||
                write(core_pipe[1], &max_freq, sizeof(double)) == -1) {
                perror("Error writing to core pipe");
                exit(EXIT_FAILURE);
            }
            close(core_pipe[1]);
            exit(0);  // Exit after one-time write
        } 
        else {
            close(core_pipe[1]);
        }
    }

    // Read cores info once
    if (cores_flag) {
        if (read(core_pipe[0], &num_cores, sizeof(int)) == -1 ||
            read(core_pipe[0], &max_freq, sizeof(double)) == -1) {
            perror("Error reading from core pipe");
        }
        close(core_pipe[0]);
    }

    // Initialize arrays
    long int memo_util_arr[samples];
    double cpu_value_arr[samples];
    long int used_mem, total_mem, overall_mem = 0;
    double cpu_usage;

    // Main sampling loop
    for (int i = 0; i < samples; i++) {
        if (mem_flag) {
            if (read(mem_pipe[0], &used_mem, sizeof(long int)) == -1 ||
                read(mem_pipe[0], &total_mem, sizeof(long int)) == -1) {
                perror("Error reading from memory pipe");
            } 
            else {
                memo_util_arr[i] = used_mem;
                overall_mem = total_mem;
            }
        }

        if (cpu_flag) {
            if (read(cpu_pipe[0], &cpu_usage, sizeof(double)) == -1) {
                perror("Error reading from CPU pipe");
            } 
            else {
                cpu_value_arr[i] = cpu_usage;
            }
        }

        graph(samples, tdelay, mem_flag, cpu_flag, memo_util_arr, overall_mem, cpu_value_arr, i);
        draw_cores(cores_flag, num_cores, max_freq);

        usleep(tdelay);
        if (sigint_triggered) {
            printf("\nDo you want to quit? [y/n]: ");
            fflush(stdout);
            char response = getchar();

            if (response == 'y' || response == 'Y') {
                should_quit = 1;
                break;
                } 
            else {
                printf("Continuing...\n");
                sigint_triggered = 0;

                // Clear remaining characters in input buffer
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
                }
            }

    }

    // Final cleanup
    if (mem_flag) {
        close(mem_pipe[0]);
        kill(mem_pid, SIGKILL);
    }
    if (cpu_flag) {
        close(cpu_pipe[0]);
        kill(cpu_pid, SIGKILL);
    }
    if (cores_flag) {
        kill(core_pid, SIGKILL);
    }

    while (wait(NULL) > 0);
    return 0;
}
