# RTKA-U Algorithm Build System
# Supports C23 standards with performance optimizations
# Author: H.Overman <opsec.ee@pm.ee>

# Compiler Configuration
CC = gcc
CSTD = -std=c2x  # C23 support (use c2x until c23 is widely available)
CFLAGS = $(CSTD) -Wall -Wextra -Wpedantic -O3 -march=native -mtune=native
LDFLAGS = -lm

# Debug and Performance Flags
DEBUG_FLAGS = -g -DDEBUG -fsanitize=address -fsanitize=undefined
PERF_FLAGS = -DNDEBUG -flto -ffast-math -funroll-loops
SIMD_FLAGS = -msse2 -mavx2 -mfma

# Target Configuration
TARGET_LIB = librtka_u.a
TARGET_TEST = rtka_u_test
TARGET_VIS = rtka_u_visualizer
TARGET_SHARED = librtka_u.so

# Source Files
ALGO_SOURCES = rtka_u.c rtka_u_plot.c
ALGO_HEADERS = rtka_u.h rtka_u_plot.h
TEST_SOURCES = rtka_u_test.c
VIS_SOURCES = rtka_u_visualizer.c
ALGO_OBJECTS = $(ALGO_SOURCES:.c=.o)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)
VIS_OBJECTS = $(VIS_SOURCES:.c=.o)

# Build Targets
.PHONY: all clean debug performance test install

# Default target: optimized build with visualization
all: $(TARGET_LIB) $(TARGET_TEST) $(TARGET_VIS)

# Build static library
$(TARGET_LIB): $(ALGO_OBJECTS)
	@echo "Building static library: $@"
	ar rcs $@ $^
	ranlib $@

# Build shared library
shared: $(TARGET_SHARED)

$(TARGET_SHARED): $(ALGO_OBJECTS)
	@echo "Building shared library: $@"
	$(CC) -shared -fPIC $(CFLAGS) $(SIMD_FLAGS) -o $@ $^ $(LDFLAGS)

# Build test executable
$(TARGET_TEST): $(TEST_OBJECTS) $(TARGET_LIB)
	@echo "Building test executable: $@"
	$(CC) $(CFLAGS) $(SIMD_FLAGS) -o $@ $(TEST_OBJECTS) -L. -lrtka_u $(LDFLAGS)

# Build visualization executable
$(TARGET_VIS): $(VIS_OBJECTS) $(TARGET_LIB)
	@echo "Building visualization executable: $@"
	$(CC) $(CFLAGS) $(SIMD_FLAGS) -o $@ $(VIS_OBJECTS) -L. -lrtka_u $(LDFLAGS)

# Object file compilation with SIMD support
%.o: %.c $(ALGO_HEADERS)
	@echo "Compiling: $< → $@"
	$(CC) $(CFLAGS) $(SIMD_FLAGS) -c $< -o $@

# Debug build target
debug: CFLAGS += $(DEBUG_FLAGS)
debug: clean $(TARGET_LIB) $(TARGET_TEST)
	@echo "Debug build completed with sanitizers enabled"

# Performance optimized build
performance: CFLAGS += $(PERF_FLAGS)
performance: clean $(TARGET_LIB) $(TARGET_TEST)
	@echo "Performance build completed with LTO and optimizations"

# Run comprehensive tests
test: $(TARGET_TEST)
	@echo "Running RTKA-U test suite..."
	./$(TARGET_TEST)

# Run interactive visualization system
visualize: $(TARGET_VIS)
	@echo "Starting RTKA-U visualization system..."
	./$(TARGET_VIS)

# Generate comprehensive analysis dashboard
dashboard: $(TARGET_VIS)
	@echo "Generating comprehensive analysis dashboard..."
	./$(TARGET_VIS) <<< "6"

# Run with performance analysis
benchmark: performance
	@echo "Running performance benchmarks..."
	time ./$(TARGET_TEST)

# Memory leak check (requires valgrind)
memcheck: debug
	@echo "Running memory leak analysis..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TARGET_TEST)

# Static analysis (requires cppcheck)
analyze:
	@echo "Running static analysis..."
	cppcheck --enable=all --inconclusive --std=c11 $(ALGO_SOURCES) $(TEST_SOURCES)

# Generate assembly output for inspection
assembly: $(ALGO_SOURCES)
	@echo "Generating optimized assembly..."
	$(CC) $(CFLAGS) $(PERF_FLAGS) $(SIMD_FLAGS) -S -fverbose-asm -o rtka_u.s rtka_u.c

# Code coverage analysis
coverage: CFLAGS += -fprofile-arcs -ftest-coverage
coverage: LDFLAGS += -lgcov
coverage: clean $(TARGET_LIB) $(TARGET_TEST)
	@echo "Building with coverage instrumentation..."
	./$(TARGET_TEST)
	gcov $(ALGO_SOURCES)
	@echo "Coverage files generated (.gcov)"

# Installation target
install: $(TARGET_LIB) $(ALGO_HEADERS)
	@echo "Installing RTKA-U library..."
	install -d /usr/local/lib
	install -d /usr/local/include
	install -m 644 $(TARGET_LIB) /usr/local/lib/
	install -m 644 $(ALGO_HEADERS) /usr/local/include/
	ldconfig

# Clean build artifacts
clean:
	@echo "Cleaning build artifacts..."
	rm -f $(ALGO_OBJECTS) $(TEST_OBJECTS) $(VIS_OBJECTS)
	rm -f $(TARGET_LIB) $(TARGET_SHARED) $(TARGET_TEST) $(TARGET_VIS)
	rm -f *.gcov *.gcda *.gcno
	rm -f *.s
	rm -f core dump.*
	rm -f rtka_u_analysis_dashboard.txt

# Display build information
info:
	@echo "RTKA-U Build Configuration:"
	@echo "  Compiler: $(CC)"
	@echo "  C Standard: $(CSTD)"
	@echo "  Optimization: $(CFLAGS)"
	@echo "  SIMD Support: $(SIMD_FLAGS)"
	@echo "  Link Flags: $(LDFLAGS)"
	@echo ""
	@echo "Available targets:"
	@echo "  all        - Build optimized library, test, and visualization tools (default)"
	@echo "  debug      - Build with debug symbols and sanitizers"
	@echo "  performance - Build with maximum optimizations"
	@echo "  shared     - Build shared library"
	@echo "  test       - Run test suite"
	@echo "  visualize  - Run interactive visualization system"
	@echo "  dashboard  - Generate comprehensive analysis dashboard"
	@echo "  benchmark  - Run performance benchmarks"
	@echo "  memcheck   - Run memory leak analysis"
	@echo "  analyze    - Run static code analysis"
	@echo "  assembly   - Generate optimized assembly output"
	@echo "  coverage   - Build and run coverage analysis"
	@echo "  install    - Install library system-wide"
	@echo "  clean      - Remove build artifacts"

# Compiler and dependency checks
check-compiler:
	@echo "Checking compiler capabilities..."
	@$(CC) --version
	@echo -n "C23 support: "
	@echo 'int main(){return 0;}' | $(CC) $(CSTD) -x c - -o /dev/null 2>/dev/null && echo "✓ Available" || echo "✗ Not available"
	@echo -n "SIMD support: "
	@echo 'int main(){return 0;}' | $(CC) $(SIMD_FLAGS) -x c - -o /dev/null 2>/dev/null && echo "✓ Available" || echo "✗ Not available"

# Help target
help: info

# Dependency generation (for development)
depend: $(ALGO_SOURCES) $(TEST_SOURCES)
	@echo "Generating dependencies..."
	$(CC) $(CFLAGS) -MM $^ > .depend

# Include dependencies if available
-include .depend
