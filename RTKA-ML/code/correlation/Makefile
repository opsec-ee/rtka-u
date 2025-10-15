# Makefile for RTKA Correlation Analysis
# Copyright (c) 2025 - H.Overman opsec.ee@pm.me
# Email: opsec.ee@pm.me
#
# Build system for correlation library with C23 standards and optimization

# Compiler and flags
CC = gcc
CFLAGS = -std=gnu2x -Wall -Wextra -Wpedantic -O3 -march=native -ffast-math
LDFLAGS = -lm

# Debug flags (use with: make DEBUG=1)
ifdef DEBUG
    CFLAGS += -g -O0 -DDEBUG
endif

# Sanitizer flags (use with: make SANITIZE=1)
ifdef SANITIZE
    CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
    LDFLAGS += -fsanitize=address,undefined
endif

# Source files
HEADERS = rtka_correlation.h
SOURCES = rtka_correlation.c
OBJECTS = rtka_correlation.o

# Test files
TEST_SOURCES = rtka_correlation_test.c
TEST_OBJECTS = rtka_correlation_test.o
TEST_BINARY = rtka_correlation_test

# Library
LIBRARY = librtka_correlation.a

# Default target
.PHONY: all
all: $(LIBRARY) $(TEST_BINARY)

# Build static library
$(LIBRARY): $(OBJECTS)
	ar rcs $@ $^
	@echo "Built static library: $@"

# Build test executable
$(TEST_BINARY): $(TEST_OBJECTS) $(LIBRARY)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Built test binary: $@"

# Compile object files
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

# Run tests
.PHONY: test
test: $(TEST_BINARY)
	@echo "========================================="
	@echo "Running correlation tests..."
	@echo "========================================="
	./$(TEST_BINARY)

# Run tests with valgrind (memory leak detection)
.PHONY: valgrind
valgrind: $(TEST_BINARY)
	@echo "Running valgrind memory check..."
	valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes ./$(TEST_BINARY)

# Clean build artifacts
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TEST_OBJECTS) $(LIBRARY) $(TEST_BINARY)
	@echo "Cleaned build artifacts"

# Install library (optional)
PREFIX ?= /usr/local
.PHONY: install
install: $(LIBRARY) $(HEADERS)
	install -d $(PREFIX)/lib
	install -m 644 $(LIBRARY) $(PREFIX)/lib/
	install -d $(PREFIX)/include
	install -m 644 $(HEADERS) $(PREFIX)/include/
	@echo "Installed to $(PREFIX)"

# Uninstall library
.PHONY: uninstall
uninstall:
	rm -f $(PREFIX)/lib/$(LIBRARY)
	rm -f $(PREFIX)/include/rtka_correlation.h
	@echo "Uninstalled from $(PREFIX)"

# Help target
.PHONY: help
help:
	@echo "RTKA Correlation Analysis - Build System"
	@echo "========================================="
	@echo "Targets:"
	@echo "  all        - Build library and tests (default)"
	@echo "  test       - Build and run tests"
	@echo "  clean      - Remove build artifacts"
	@echo "  install    - Install library to PREFIX (default: /usr/local)"
	@echo "  uninstall  - Uninstall library from PREFIX"
	@echo "  valgrind   - Run tests with valgrind memory checker"
	@echo ""
	@echo "Options:"
	@echo "  DEBUG=1    - Build with debug symbols and no optimization"
	@echo "  SANITIZE=1 - Build with address and undefined behavior sanitizers"
	@echo "  PREFIX=dir - Set installation directory (default: /usr/local)"
	@echo ""
	@echo "Examples:"
	@echo "  make              # Build everything"
	@echo "  make test         # Run tests"
	@echo "  make DEBUG=1 test # Build and run tests with debugging"
	@echo "  make clean        # Clean build artifacts"
