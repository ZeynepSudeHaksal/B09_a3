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

#define READ_END 0
#define WRITE_END 1

volatile sig_atomic_t should_quit = 0;
volatile sig_atomic_t sigint_triggered = 0;

pid_t mem_pid = -1, cpu_pid = -1, core_pid = -1;
int mem_pipe[2], cpu_pipe[2], core_pipe[2];

void handle_sigint(int sig) {
    sigint_triggered = 1;
}

void handle_sigtstp(int sig) {
    char msg[] = "This is ctrl z, Caught signal 20\n";
    write(STDOUT_FILENO, msg, sizeof(msg) - 1);
}

int main(int argc, char *argv[]) {
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    int samples = 20;
    int tdelay = 500000;
    int mem_flag = 0, cpu_flag = 0, cores_flag = 0;

    for (int i = 1; i < argc; i++) {
        if (i == 1 && isdigit(*argv[i])) samples = atoi(argv[i]);
        else if (i == 2 && isdigit(*argv[i])) tdelay = atoi(argv[i]);
        else if (strcmp(argv[i], "--memory") == 0) mem_flag = 1;
        else if (strcmp(argv[i], "--cpu") == 0) cpu_flag = 1;
        else if (strcmp(argv[i], "--cores") == 0) cores_flag = 1;
    }

    if (!mem_flag && !cpu_flag && !cores_flag) mem_flag = cpu_flag = cores_flag = 1;

    if (mem_flag && pipe(mem_pipe) == -1) {
        perror("Error creating memory pipe");
        exit(EXIT_FAILURE);
    }
    if (cpu_flag && pipe(cpu_pipe) == -1) {
        perror("Error creating CPU pipe");
        exit(EXIT_FAILURE);
    }
    if (cores_flag && pipe(core_pipe) == -1) {
        perror("Error creating core pipe");
        exit(EXIT_FAILURE);
    }

    // === Memory Process ===
    if (mem_flag) {
        mem_pid = fork();
        if (mem_pid == -1) {
            perror("Error forking memory process");
            exit(EXIT_FAILURE);
        } else if (mem_pid == 0) {
            close(mem_pipe[READ_END]);
            while (1) {
                MemInfo mem;
                if (read_mem_info(&mem) != 0 || mem.total == 0) {
                    fprintf(stderr, "Memory info read failed\n");
                    exit(EXIT_FAILURE);
                }

                double used_percent = 100.0 * (mem.total - mem.free) / mem.total;

                if (write(mem_pipe[WRITE_END], &used_percent, sizeof(double)) == -1 ||
                    write(mem_pipe[WRITE_END], &mem.total, sizeof(long int)) == -1) {
                    perror("Error writing to memory pipe");
                    exit(EXIT_FAILURE);
                }
                usleep(tdelay);
            }
        } else {
            close(mem_pipe[WRITE_END]);
        }
    }

    // === CPU Process ===
    if (cpu_flag) {
        cpu_pid = fork();
        if (cpu_pid == -1) {
            perror("Error forking CPU process");
            exit(EXIT_FAILURE);
        } else if (cpu_pid == 0) {
            close(cpu_pipe[READ_END]);
            double cpu;
            while (1) {
                cpu = calculate_cpu_utilization();
                if (write(cpu_pipe[WRITE_END], &cpu, sizeof(double)) == -1) {
                    perror("Error writing to CPU pipe");
                    exit(EXIT_FAILURE);
                }
                usleep(tdelay);
            }
        } else {
            close(cpu_pipe[WRITE_END]);
        }
    }

    // === Cores Process ===
    int num_cores = 0, max_freq = 0;
    if (cores_flag) {
        core_pid = fork();
        if (core_pid == -1) {
            perror("Error forking cores process");
            exit(EXIT_FAILURE);
        } else if (core_pid == 0) {
            close(core_pipe[READ_END]);
            get_core_info(&num_cores, &max_freq);
            if (write(core_pipe[WRITE_END], &num_cores, sizeof(int)) == -1 ||
                write(core_pipe[WRITE_END], &max_freq, sizeof(int)) == -1) {
                perror("Error writing to core pipe");
                exit(EXIT_FAILURE);
            }
            close(core_pipe[WRITE_END]);
            exit(0);
        } else {
            close(core_pipe[WRITE_END]);
        }
    }

    if (cores_flag) {
        if (read(core_pipe[READ_END], &num_cores, sizeof(int)) == -1 ||
            read(core_pipe[READ_END], &max_freq, sizeof(int)) == -1) {
            perror("Error reading from core pipe");
        }
        close(core_pipe[READ_END]);
    }

    double memo_util_arr[samples];
    double cpu_value_arr[samples];
    double mem_usage = 0.0, cpu_usage = 0.0;
    long int overall_mem = 0;

    for (int i = 0; i < samples; i++) {
        if (mem_flag) {
            if (read(mem_pipe[READ_END], &mem_usage, sizeof(double)) == -1 ||
                read(mem_pipe[READ_END], &overall_mem, sizeof(long int)) == -1) {
                perror("Error reading from memory pipe");
            } else {
                memo_util_arr[i] = mem_usage;
            }
        }

        if (cpu_flag) {
            if (read(cpu_pipe[READ_END], &cpu_usage, sizeof(double)) == -1) {
                perror("Error reading from CPU pipe");
            } else {
                cpu_value_arr[i] = cpu_usage;
            }
        }

        graph(samples, tdelay, mem_flag, cpu_flag, cores_flag,
              memo_util_arr, overall_mem, cpu_value_arr, i);

        draw_cores(num_cores);
        usleep(tdelay);

        if (sigint_triggered) {
            printf("\nDo you really want to quit? [y/n]: ");
            fflush(stdout);
            char response = getchar();

            if (response == 'y' || response == 'Y') {
                should_quit = 1;
                break;
            } else {
                printf("Continuing...\n");
                sigint_triggered = 0;
                int ch;
                while ((ch = getchar()) != '\n' && ch != EOF);
            }
        }
    }

    if (mem_flag) {
        close(mem_pipe[READ_END]);
        kill(mem_pid, SIGKILL);
    }
    if (cpu_flag) {
        close(cpu_pipe[READ_END]);
        kill(cpu_pid, SIGKILL);
    }
    if (cores_flag) {
        kill(core_pid, SIGKILL);
    }

    while (wait(NULL) > 0);
    return 0;
}
