# Makefile for compiling second_preim_48_fillme.c with dependencies on xoshiro.h

# Compiler
CC = gcc

# Compiler flags
CFLAGS = -w -O3 -march=native

# Target executable name
TARGET = bin/second_preim_48

# Source files
SRCS = src/struct.c src/second_preim_48_fillme.c

# Object files (replace .c with .o)
OBJS = $(SRCS:.c=.o)

# Default target
all: $(TARGET)

# Rule to build the target executable
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

# Rule to compile the source files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the program
run: $(TARGET)
	./$(TARGET)

# Clean up build files
clean:
	rm -f $(OBJS) $(TARGET)

# Phony targets
.PHONY: all clean
