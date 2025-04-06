CC = gcc

CFLAGS = -Wall -g -std=c99 -Werror 

TARGET = systemMonitor

OBJECTS = a3.o memory.o cpu.o cores.o graph.o 
HEADS = memory.h cpu.h cores.h graph.h

$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c $(HEADS)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -f *.o $(TARGET)