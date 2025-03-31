#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/wait.h>

// Include the header files for each module
#include "memory.h"
#include "cpu.h"
#include "cores.h"
#include "graph.h"

void handle_sigint(int sig) {
    char answer[10];
    printf("\nDo you really want to quit? [y/n] ");
    fgets(answer, sizeof(answer), stdin);
    if (answer[0] == 'y' || answer[0] == 'Y') {
        printf("Exiting program...\n");
        exit(0);
    } else {
        printf("Continuing execution...\n");
    }
}

void handle_sigtstp(int sig) {
    printf("\nCtrl-Z pressed, but ignored.\n");
}

int main(int argc, char *argv[]) {
    int samp = 20;
    int delay = 500000;
    int mem = 0, cp = 0, core = 0;

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (isdigit(*argv[i])) {
            if (i == 1) {
                samp = atoi(argv[i]);
            } else if (i == 2) {
                delay = atoi(argv[i]);
            }
        } else if (strcmp(argv[i], "--memory") == 0) {
            mem = 1;
        } else if (strcmp(argv[i], "--cpu") == 0) {
            cp = 1;
        } else if (strcmp(argv[i], "--cores") == 0) {
            core = 1;
        } else if (strncmp(argv[i], "--samples=", 10) == 0) {
            samp = atoi(argv[i] + 10);
        } else if (strncmp(argv[i], "--tdelay=", 9) == 0) {
            delay = atoi(argv[i] + 9);
        } else {
            fprintf(stderr, "Invalid argument: %s\n", argv[i]);
            return 1;
        }
    }

    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);

    // Setup pipes
    int pipes[3][2];
    for (int i = 0; i < 3; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("Failed to create pipes");
            return 1;
        }
        printf("Pipe %d created successfully.\n", i);
    }

    // Fork processes
    for (int i = 0; i < 3; i++) {
        if (fork() == 0) {
            close(pipes[i][0]); // Close read end in child

            printf("Child process %d started for ", i);
            if (i == 0 && mem) { // Memory process
                printf("memory monitoring.\n");
                long int used_memory, total_memory;
                get_memory_usage(&used_memory, &total_memory);
                write(pipes[i][1], &used_memory, sizeof(used_memory));
                write(pipes[i][1], &total_memory, sizeof(total_memory));
            } else if (i == 1 && cp) { // CPU process
                printf("CPU monitoring.\n");
                long int total, idle;
                read_cpu_times(&total, &idle);
                write(pipes[i][1], &total, sizeof(total));
                write(pipes[i][1], &idle, sizeof(idle));
            } else if (i == 2 && core) { // Cores process
                printf("core monitoring.\n");
                int num_cores, max_freq;
                get_core_info(&num_cores, &max_freq);
                write(pipes[i][1], &num_cores, sizeof(num_cores));
                write(pipes[i][1], &max_freq, sizeof(max_freq));
            }
            close(pipes[i][1]); // Close write end
            exit(0);
        } else {
            close(pipes[i][1]); // Close write end in parent
            printf("Parent closed write end for pipe %d.\n", i);
        }
    }

    // Read from pipes in parent
    int num_cores, max_freq;
    long int used_memory, total_memory, total, idle;
    if (mem) {
        read(pipes[0][0], &used_memory, sizeof(used_memory));
        read(pipes[0][0], &total_memory, sizeof(total_memory));
        printf("Memory data read: used %ld kB, total %ld kB.\n", used_memory, total_memory);
    }
    if (cp) {
        read(pipes[1][0], &total, sizeof(total));
        read(pipes[1][0], &idle, sizeof(idle));
        printf("CPU data read: total %ld, idle %ld.\n", total, idle);
    }
    if (core) {
        read(pipes[2][0], &num_cores, sizeof(num_cores));
        read(pipes[2][0], &max_freq, sizeof(max_freq));
        printf("Core data read: number of cores %d, max frequency %d MHz.\n", num_cores, max_freq);
    }

    // Wait for all child processes to finish
    for (int i = 0; i < 3; i++) {
        wait(NULL);
        printf("Child process %d has finished.\n", i);
    }

    // Display data (for simplicity, assume we always display all)
    printf("Memory used: %ld kB, Total memory: %ld kB\n", used_memory, total_memory);
    printf("CPU total: %ld, CPU idle: %ld\n", total, idle);
    printf("Number of cores: %d, Max frequency: %d MHz\n", num_cores, max_freq);

    return 0;
}
