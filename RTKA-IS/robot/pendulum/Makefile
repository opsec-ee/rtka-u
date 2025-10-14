# Makefile for RTKA Robotics Control System
# Copyright (c) 2025 - H.Overman opsec.ee@pm.me
# Email: opsec.ee@pm.me

# Compiler
CC = gcc

# C23 standard where possible, C11 as fallback
CFLAGS = -std=c2x -Wall -Wextra -Wpedantic -O3 -march=native
CFLAGS += -ffast-math -fno-signed-zeros -fno-trapping-math
CFLAGS += -D_POSIX_C_SOURCE=200809L

# Math library
LDFLAGS = -lm

# Source files
SRC_CONTROL = rtka_robotics_control.c
SRC_SIM = rtka_pendulum_sim.c

# Object files
OBJ_CONTROL = rtka_robotics_control.o
OBJ_SIM = rtka_pendulum_sim.o

# Executables
TARGET_SIM = rtka_pendulum_sim

# Default target
all: $(TARGET_SIM)

# Compile control system
$(OBJ_CONTROL): $(SRC_CONTROL) rtka_robotics_control.h
	$(CC) $(CFLAGS) -c $(SRC_CONTROL) -o $(OBJ_CONTROL)

# Compile simulation
$(OBJ_SIM): $(SRC_SIM) rtka_robotics_control.h
	$(CC) $(CFLAGS) -c $(SRC_SIM) -o $(OBJ_SIM)

# Link simulation executable
$(TARGET_SIM): $(OBJ_SIM) $(OBJ_CONTROL)
	$(CC) $(CFLAGS) $(OBJ_SIM) $(OBJ_CONTROL) $(LDFLAGS) -o $(TARGET_SIM)

# Run simulation
run: $(TARGET_SIM)
	./$(TARGET_SIM)

# Clean build artifacts
clean:
	rm -f $(OBJ_CONTROL) $(OBJ_SIM) $(TARGET_SIM)

# Rebuild from scratch
rebuild: clean all

# Check for memory leaks (requires valgrind)
valgrind: $(TARGET_SIM)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_SIM)

# Static analysis (requires cppcheck)
analyze:
	cppcheck --enable=all --suppress=missingIncludeSystem $(SRC_CONTROL) $(SRC_SIM)

# Format code (requires clang-format)
format:
	clang-format -i $(SRC_CONTROL) $(SRC_SIM) rtka_robotics_control.h

.PHONY: all run clean rebuild valgrind analyze format
