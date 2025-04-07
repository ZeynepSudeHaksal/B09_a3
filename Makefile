CC = gcc
CFLAGS = -Wall -g -std=c99 -Werror -D_DEFAULT_SOURCE

TARGET = systemMonitor

OBJECTS = a3.o memory.o cpu.o cores.o graph.o

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^ -lm

a3.o: a3.c memory.h cpu.h cores.h graph.h
	$(CC) $(CFLAGS) -c $< -o $@

memory.o: memory.c memory.h
	$(CC) $(CFLAGS) -c $< -o $@

cpu.o: cpu.c cpu.h
	$(CC) $(CFLAGS) -c $< -o $@

cores.o: cores.c cores.h
	$(CC) $(CFLAGS) -c $< -o $@

graph.o: graph.c graph.h
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean run

clean:
	rm -f *.o $(TARGET)