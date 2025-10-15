# Makefile for pthread_stack program

# Compiler and flags
CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -pthread
LDFLAGS = -pthread

# Target executable
TARGET = pthread_stack

# Source files
SOURCES = pthread_stack.c

# Object files (automatically generated from SOURCES)
OBJECTS = $(SOURCES:.c=.o)

# Default target
all: $(TARGET)

# Build the executable
$(TARGET): $(OBJECTS)
	$(CC) $(OBJECTS) -o $(TARGET) $(LDFLAGS)

# Compile source files to object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Clean up generated files
clean:
	rm -f $(OBJECTS) $(TARGET)

# Rebuild everything
rebuild: clean all

# Run the program with default 4 threads
run: $(TARGET)
	./$(TARGET) 4

# Run with custom number of threads (usage: make run-custom THREADS=8)
run-custom: $(TARGET)
	./$(TARGET) $(THREADS)

# Phony targets (not actual files)
.PHONY: all clean rebuild run run-custom