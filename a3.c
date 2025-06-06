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
    ///_|> descry: handles SIGINT (Ctrl+C) by setting a flag to trigger a quit confirmation
    ///_|> sig: signal number 
    ///_|> returning: this function does not return anything

   sigint_triggered = 1;
}

// Handle Ctrl+Z
void handle_sigtstp(int sig) {
    ///_|> descry: handles SIGTSTP (Ctrl+Z) by printing a caught message and ignoring the signal
    ///_|> sig: signal number 
    ///_|> returning: this function does not return anything
    char msg[] = "This is ctrl z, continuing\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);

}

int main(int argc, char *argv[]) {
    ///_|> descry: entry point for the system monitor program, handles process forking, data sampling, and plotting
    ///_|> argc: number of command line arguments
    ///_|> argv: array of command line argument strings
    ///_|> returning: program exit status

    //signal handling
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    // Default sample size and delay
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

    // Enable all flags by default if none are set
    if (!mem_flag && !cpu_flag && !cores_flag) {
        mem_flag = cpu_flag = cores_flag = 1;
    }
    pid_t mem_pid = -1, cpu_pid = -1, core_pid = -1;
    int mem_pipe[2], cpu_pipe[2], core_pipe[2];

    // Memory pipe
    if (mem_flag && pipe(mem_pipe) == -1) {
        perror("Error creating memory pipe");
        exit(1);
    }

    // CPU pipe
    if (cpu_flag && pipe(cpu_pipe) == -1) {
        perror("Error creating CPU pipe");
        exit(1);
    }

    // Core pipe
    if (cores_flag && pipe(core_pipe) == -1) {
        perror("Error creating core pipe");
        exit(1);
    }

    // Fork memory process
    if (mem_flag) {
        mem_pid = fork();
        if (mem_pid == -1) {
            perror("Error forking memory process");
            exit(1);
        } 
        else if (mem_pid == 0) {
            close(mem_pipe[0]);
            long int used, total;
            while (1) { //continue getting memory information until the child terminates for real-time updates
                get_memory_usage(&used, &total);
                if (write(mem_pipe[1], &used, sizeof(long int)) == -1 || write(mem_pipe[1], &total, sizeof(long int)) == -1) {
                    perror("Error writing to memory pipe");
                    exit(1);
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
            exit(1);
        } 
        else if (cpu_pid == 0) {
            close(cpu_pipe[0]);

            CpuTimes prev, curr;
            double cpu;

            if (read_cpu_times(&prev) != 0) {
                fprintf(stderr, "Initial CPU read failed\n");
                exit(1);
            }

            usleep(tdelay);  // wait before the first diff

            while (1) { //continue getting cpu information until the child terminates for real-time updates
                if (read_cpu_times(&curr) != 0) {
                    fprintf(stderr, "CPU read failed\n");
                    exit(1);
                }

                cpu = calculate_cpu_utilization(&prev, &curr);

                if (write(cpu_pipe[1], &cpu, sizeof(double)) == -1) {
                    perror("Error writing to CPU pipe");
                    exit(1);
                }

                prev = curr; // update previous
                usleep(tdelay);
            }
        } 
        else {
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
            exit(1);
        } 
        else if (core_pid == 0) {
            close(core_pipe[0]);
            get_core_info(&num_cores, &max_freq);
            if (write(core_pipe[1], &num_cores, sizeof(int)) == -1 || write(core_pipe[1], &max_freq, sizeof(double)) == -1) {
                perror("Error writing to core pipe");
                exit(1);
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

    while (wait(NULL) > 0); // to prevent zombie processes
    return 0;
}
