# B09_a3

## Assignment 3 for CSCB09 - Winter 2025
Author: Zeynep Sude Haksal
Date: April 7 2025

### Introduction
This project is a system monitoring tool  implemented in C. It tracks and displays system memory usage, CPU utilization, and CPU core details over a user-defined period. The main goal is to visualize how system resources are being used in real-time usin graphs, while also allowing users to customize the metrics they want to observe. It is targeted to run on a Linux-based system.

### Description of how you solve/approach the problem

To solve the problem, I used a modular and multi-process approach:

- I began by setting up the main logic in a3.c to handle user input, signal interruptions, and fork child processes.

- For each one of memory, cpu and cores, I created separate C modules (memory.c, cpu.c, cores.c) to encapsulate the functionality and make a more modular and clean code.

- Pipes were established for inter-process communication, where child processes collected data and sent it back to the parent.

- The graph.c module handles the visualization of cpu, memory and core information.

- I used system files like /proc/meminfo, /proc/stat, and /proc/cpuinfo to read resource data.

- I handled SIGINT and SIGTSTP signals to allow exit or continuation.


### Implementation
- The main loop is in a3.c. It handles sampling data and coordinating child processes via pipes.

- Each module reads from a specific system file and processes data accordingly.

- Graphs are drawn in the terminal to visualize the memory and CPU usage.

- The program also handles user interruption (Ctrl+C) to allow early exit.

1. Main (a3.c):
- Handles command-line arguments, signal handling, and controls the flow of the program. Initially sets all metric flags (memory, CPU, cores) to zero. If no specific metric is requested, it defaults to enabling all. It parses the command-line arguments for sample size, delay, and feature flags. Depending on the flags, it forks separate child processes for each resource monitor and sets up pipes for inter-process communication. The main loop reads from the pipes, updates corresponding data arrays, and calls the visualization functions. It also handles SIGINT to optionally allow the user to quit early.
    - handle_sigint(int sig)
    Captures the SIGINT signal (Ctrl+C) and sets a flag (sigint_triggered = 1) to later prompt the user whether they want to exit. This allows for non-disruptive interruption. I saw on Piazza that we can use volatile sig_atomic_t for signal handling. So I defined two flags signint_triggered and should_quit. Once (Ctrl+C) is detected handle_signit function sets signint_triggered to 1. In the main function, it checks whether signint_triggered is 0 or 1, if it is 1, it asks user if they want to quit. If the answer is yes it sets should_quit to 1 and the program safely terminates. 

    - handle_sigtstp(int sig)
    Captures the SIGTSTP signal (Ctrl+Z) and prints a message stating that the signal was caught. It continues the program without terminating.
2. Memory Monitoring (memory.c)
Responsible for reading and calculating memory usage.
-  get_memory_usage(long int *used_memory, long int *total_memory)
Opens /proc/meminfo and reads lines to extract MemTotal and MemAvailable. It calculates used memory as total - available and returns both through pointer arguments. It uses sscanf to extract numeric values from the read lines and stops once both values are found.

- calculate_memory_utilization(long int used_memory)
Converts memory from kilobytes to gigabytes by dividing the input value by 1024 twice. It is used mainly for display formatting in the terminal graph.

3. CPU Monitoring (cpu.c)
Handles reading and analyzing CPU usage over time.

- read_cpu_times(CpuTimes *times)
Opens /proc/stat and reads the first line labeled "cpu", extracting values representing time spent in various CPU modes (user, nice, system, idle, iowait, irq, softirq). These values are stored in a CpuTimes struct to track CPU state at a moment in time. It uses fscanf for structured reading.

- calculate_cpu_utilization(CpuTimes *prev, CpuTimes *curr)
Calculates the CPU utilization percentage between two snapshots of CPU time. It computes the total time difference and the idle time difference, then derives the percentage of time spent in non-idle operations. Returns a floating-point value representing CPU usage over that interval.

4. Core Information (cores.c)
Collects information about logical cores and their frequency.

- get_core_info(int *num_cores, double *max_freq)
Opens /proc/cpuinfo and counts lines starting with "processor" to determine the number of logical cores. Then it opens /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq to get the maximum frequency in kHz and converts it to MHz. Both values are returned via pointers.

5. Graphing and Visualization (graph.c)
Prints real-time graphs and CPU core representation.

- graph(int sample, int delay, int mem, int cp, long int* mem_arr, long int overall_value, double* cpu_arr, int t)
Clears the terminal using escape sequences, then prints memory and CPU graphs. For memory, it shows usage in GB and a vertical bar plot over time. For CPU, it prints usage in % and a similar bar graph. It loops through all samples up to the current time t to render each vertical level of the graph. It scales the graphs proportionally to the maximum value (100% CPU, total memory).

- draw_cores(int core, int cores, double maxfreq)
If core flag is active, prints the number of cores and their maximum frequency. Then prints a visual grid of boxes (up to 4 per row), where each box represents a core. 

### Pseudo-Code
1. Main:
Initialize signal handlers (SIGINT, SIGTSTP)

Set default values for: samples = 20, delay = 500000, and flags = 0

For each command-line argument:
    If numeric, update samples or delay
    If flag (--memory, --cpu, --cores), set corresponding flag to 1

If no flags set, enable all three flags by default

If memory flag set:
    Create memory pipe
    Fork memory process
        Child:
            Loop:
                Get memory usage
                Write used and total to pipe
                Sleep for delay

If CPU flag set:
    Create CPU pipe
    Fork CPU process
        Child:
            Read initial CPU times
            Sleep for delay
            Loop:
                Read current CPU times
                Calculate utilization
                Write utilization to pipe
                Sleep for delay

If cores flag set:
    Create core pipe
    Fork cores process
        Child:
            Get core info
            Write to pipe once and exit

In parent:
    If cores enabled, read core data from pipe

    Initialize memory and CPU arrays

    Loop for each sample:
        If memory enabled, read from pipe and store in an array
        If CPU enabled, read from pipe and store in an array
        Call graph() to draw memory and/or CPU usage
        Call draw_cores() to show core grid

        If SIGINT triggered:
            Ask user to quit or continue

    After loop:
        Close pipes, kill child processes
        Wait for all children to exit (to prevent zombie processes)

- handle_sigint(sig):
    Set sigint_triggered = 1

- handle_sigtstp(sig)
    Print message: "This is ctrl z, continuing"

2. Memory Monitoring (memory.c)
- get_memory_usage(used_memory, total_memory):

Open /proc/meminfo

Loop through each line:
    If line has "MemTotal:", extract and store
    If line has "MemAvailable:", extract and store
    Break when both found

Calculate used_memory = total_memory - available

Close file

- calculate_memory_utilization(used_memory):
    Return used_memory divided by 1024 twice (KB → GB)

3.  CPU Monitoring (cpu.c)
- read_cpu_times(times):

Open /proc/stat

Read line starting with "cpu"
    Parse and store user, nice, system, idle, iowait, irq, softirq in struct

Close file
Return 0 if successful, -1 otherwise

- calculate_cpu_utilization(prev, curr):

Calculate total time from each field in both prev and curr
Calculate idle time from idle fields

total_diff = curr_total - prev_total
idle_diff = curr_idle - prev_idle

If total_diff == 0:
    Return 0.0

Return (total_diff - idle_diff) / total_diff * 100

4.  Core Info (cores.c):
- get_core_info(num_cores, max_freq):

Open /proc/cpuinfo

Loop through lines:
    If line starts with "processor", increment core count

Store count in *num_cores

Open /sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq
Read and store value in *max_freq

Close both files

5. Graphing (graph.c)
- graph(sample, delay, mem, cp, mem_arr, overall_value, cpu_arr, t):

Clear terminal screen

Print sampling rate and delay

If memory flag:
    Print current memory usage in GB

    For each vertical level (12 to 1):
        Print bar if mem_arr[i] scaled equals current level

    Print x-axis and memory labels

If CPU flag:
    Print current CPU usage in %

    For each vertical level (10 to 1):
        Print bar if cpu_arr[i] scaled equals current level

    Print x-axis and CPU labels

- draw_cores(core, cores, max_freq):

If core flag not set, return

Print number of cores and frequency

Calculate number of rows (4 cores per row)

For each row:
    Print top border for up to 4 cores
    Print core box (|    |)
    Print bottom border


### How to Run the Program

1. Compilation: To compile the program using the Makefile, run the following command in the terminal:

   make 

2. Running the Program: Once compiled, the program can be executed with the following syntax:

   ./myMonitoringTool [samples [tdelay]] [--memory] [--cpu] [--cores] [--samples=N] [--tdelay=T]

   - `samples`: Number of times the statistics will be collected (default is 20).
   - `tdelay`: Sampling frequency in microseconds (default is 500,000 microseconds or 0.5 seconds).
   - `--memory`: Reports memory utilization.
   - `--cpu`: Reports CPU utilization.
   - `--cores`: Reports core information.
   - `--samples=N`: Specifies the number of samples to collect (overrides the default).
   - `--tdelay=T`: Specifies the delay between samples in microseconds (overrides the default).

   Examples:

   - To display memory, CPU, and core utilization every second with 10 samples:
     ./myMonitoringTool 10 1000000 --memory --cpu --cores
   - To display only CPU utilization with 30 samples and a delay of 0.3 seconds:
     ./myMonitoringTool 30 300000 --cpu


### Expected Output

v Memory 2.34 GB
16 GB |   
      |
      | #
0 GB  _______________________

v CPU 34.56 %
100% |   
     |
     | :
0%    _______________________

Number of Cores: 8 @ 3.20 GHz
+----+  +----+  +----+  +----+
|    |  |    |  |    |  |    |
+----+  +----+  +----+  +----+
...

- The program prints an bar graph of memory (in GB) and CPU (in %) usage.

- It displays CPU core count and frequency in a grid format.

- Output updates in real-time based on delay and sample count.

- If user presses Ctrl+C, they're asked whether to quit or not.
- If user presses Ctrl+Z, the signal is ignored.

### Test Cases
Case Input Args	                    Expected Behavior
1	./myMonitoringTool	            Default 20 samples, all metrics
2	./myMonitoringTool 5 1000000	5 samples, 1s delay, all metrics
3	./myMonitoringTool --memory	    Memory graph only (default values for sample and delay)
4	./myMonitoringTool --cpu	    CPU graph only (defauit values for sample and delay)
5	./myMonitoringTool --cores	    Core display only (defauit values for sample and delay)
6	Mistyped args (--mem)	        Defaults to all enabled
7	Ctrl+C mid-run	                Prompt user to quit or continue
8	Ctrl+Z mid-run	                Ignore signal and continue


### Disclaimers
- The program retrieves system statistics by reading the contents of files under `/proc/` (like `/proc/meminfo`, `/proc/stat`, `/proc/cpuinfo`), and the program assumes these files are present and contain valid information.
- The number of cores is determined by reading specific values from `/proc/cpuinfo`, and the maximum CPU frequency is read from `/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq`.
- The program assumes that the content of the files do not change (except the values it reads). In other words, lines, characters besides the read values, are assumed constant and used in calculations to reach to the wanted values. Also the wordings of the CLA's are also assumed to be not changing.
- This program uses usleep() do add the delays (with the flag tdelay and the respective variable delay). Users should be careful about which type of machines and versions of C accept usleep().
- All 3 graphs (Memory, CPU, Cores) are written at the same time.
- In the Makefile -D_DEFAULT_SOURCE flag is used because it wouldn't compile due to usleep() function
- In the Makefile -lm is used because it wouldn't link due to the ceil() function
- I saw on Piazza that we can use volatile sig_atomic_t for signal handling. So I defined two flags signint_triggered and should_quit. I explained how they are used in the implementation section.
- When Ctrl+Z is pressed it writes a message to the terminal however since the terminal is cleared every 0.5 seconds, it is not visable.


### Resources
- https://piazza.com/class/m5j5smmmxyx2s0/post/277
- https://stackoverflow.com/questions/7794273/c-using-signals-to-stop-child-processes
- https://man7.org/linux/man-pages/man2/kill.2.html
- https://man7.org/linux/man-pages/man2/sigaction.2.html
- https://en.cppreference.com/w/c/program/sig_atomic_t
- https://stackoverflow.com/questions/10053788/implicit-declaration-of-function-usleep
- https://www.linuxhowtos.org/System/procstat.htm
- https://docs.kernel.org/filesystems/proc.html
- https://opensource.com/article/18/8/what-how-makefile
- https://www.gnu.org/software/make/manual/make.html

