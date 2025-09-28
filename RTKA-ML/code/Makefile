# RTKA System Makefile
# Copyright (c) 2025 - H.Overman opsec.ee@pm.me

CC = gcc
CFLAGS = -O3 -Wall -Wextra -std=c11 -march=native -ffast-math -Wno-maybe-uninitialized
LDFLAGS = -lm -lpthread
AR = ar
ARFLAGS = rcs

# Directories
BUILD_DIR = build
OBJ_DIR = $(BUILD_DIR)/obj
LIB_DIR = $(BUILD_DIR)/lib
BIN_DIR = $(BUILD_DIR)/bin

# Library
LIB_NAME = librtka.a

# Source files by level
CORE_SRCS = rtka_u_core.c
MEMORY_SRCS = rtka_memory.c
VECTOR_SRCS = rtka_vector.c
ML_FOUNDATION_SRCS = rtka_tensor.c rtka_gradient.c rtka_optimizer.c
NN_SRCS = rtka_nn.c rtka_gnn.c
GRAPH_SRCS = rtka_graph.c
EVOLUTION_SRCS = rtka_evolution.c
ML_APP_SRCS = rtka_ml.c rtka_reinforcement.c
SOLVER_SRCS = rtka_solver.c rtka_sudoku_729.c rtka_nqueens.c rtka_sat.c rtka_rubik.c rtka_rubik_324.c rtka_astar.c
UTIL_SRCS = rtka_random.c

# All library sources
LIB_SRCS = $(CORE_SRCS) $(MEMORY_SRCS) $(VECTOR_SRCS) $(ML_FOUNDATION_SRCS) \
           $(NN_SRCS) $(GRAPH_SRCS) $(EVOLUTION_SRCS) $(ML_APP_SRCS) \
           $(SOLVER_SRCS) $(UTIL_SRCS)

# Object files
LIB_OBJS = $(addprefix $(OBJ_DIR)/, $(LIB_SRCS:.c=.o))

# Test programs
TEST_PROGS = test_sudoku_729 test_nqueens test_sat test_rubik test_rubik_324 test_astar

# Default target
all: dirs $(LIB_DIR)/$(LIB_NAME) tests

# Create directories
dirs:
	@mkdir -p $(OBJ_DIR) $(LIB_DIR) $(BIN_DIR)

# Build static library
$(LIB_DIR)/$(LIB_NAME): $(LIB_OBJS)
	@echo "Building RTKA library..."
	$(AR) $(ARFLAGS) $@ $^
	@echo "Library built: $@"

# Compile library objects
$(OBJ_DIR)/%.o: %.c
	@echo "Compiling $<..."
	$(CC) $(CFLAGS) -c $< -o $@

# Build all tests
tests: $(addprefix $(BIN_DIR)/, $(TEST_PROGS))

$(BIN_DIR)/test_sudoku_729: test_sudoku_729.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

$(BIN_DIR)/test_nqueens: test_nqueens.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

$(BIN_DIR)/test_sat: test_sat.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

$(BIN_DIR)/test_rubik: test_rubik.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

$(BIN_DIR)/test_rubik_324: test_rubik_324.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

$(BIN_DIR)/test_astar: test_astar.c $(LIB_DIR)/$(LIB_NAME)
	$(CC) $(CFLAGS) $< -L$(LIB_DIR) -lrtka $(LDFLAGS) -o $@

# Run individual tests
run_sudoku: $(BIN_DIR)/test_sudoku_729
	$(BIN_DIR)/test_sudoku_729

run_nqueens: $(BIN_DIR)/test_nqueens
	$(BIN_DIR)/test_nqueens

run_sat: $(BIN_DIR)/test_sat
	$(BIN_DIR)/test_sat

run_rubik: $(BIN_DIR)/test_rubik
	$(BIN_DIR)/test_rubik

run_rubik_324: $(BIN_DIR)/test_rubik_324
	$(BIN_DIR)/test_rubik_324

run_astar: $(BIN_DIR)/test_astar
	$(BIN_DIR)/test_astar

# Run all tests
run_all: tests
	@echo "Running all RTKA tests..."
	@for test in $(TEST_PROGS); do \
		echo "\n=== Running $$test ===" && $(BIN_DIR)/$$test; \
	done

# Clean build files
clean:
	rm -rf $(BUILD_DIR)

# Debug build
debug: CFLAGS = -g -O0 -Wall -Wextra -std=c11 -DDEBUG
debug: all

# Profile build
profile: CFLAGS += -pg
profile: LDFLAGS += -pg
profile: all

# Help
help:
	@echo "RTKA Build System"
	@echo "================="
	@echo "Targets:"
	@echo "  all          - Build library and tests (default)"
	@echo "  tests        - Build test programs"
	@echo "  run_sudoku   - Run Sudoku solver test"
	@echo "  run_nqueens  - Run N-Queens solver test"
	@echo "  run_sat      - Run SAT solver test"
	@echo "  run_rubik    - Run Rubik's cube solver test"
	@echo "  run_rubik_324- Run 324-state Rubik's solver test"
	@echo "  run_astar    - Run A* pathfinding test"
	@echo "  run_all      - Run all tests"
	@echo "  clean        - Remove build files"
	@echo "  debug        - Build with debug symbols"
	@echo "  profile      - Build with profiling"
	@echo "  help         - Show this help"

.PHONY: all dirs tests clean debug profile help
.PHONY: run_sudoku run_nqueens run_sat run_rubik run_rubik_324 run_astar run_all
