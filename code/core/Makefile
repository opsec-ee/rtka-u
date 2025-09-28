# Makefile for RTKA Core System
# Copyright (c) 2025 - H.Overman <opsec.ee@pm.me>
# Email: opsec.ee@pm.me

# Compiler and flags
CC = gcc
CSTD = -std=c23
CFLAGS = -Wall -Wextra -Wpedantic -Werror -O2 -g
CFLAGS_DEBUG = -Wall -Wextra -Wpedantic -O0 -g -DDEBUG -fsanitize=address,undefined
CFLAGS_RELEASE = -Wall -Wextra -Wpedantic -Werror -O3 -DNDEBUG -march=native
CFLAGS_PROF = -Wall -Wextra -Wpedantic -O2 -g -pg

# Directories
SRCDIR = .
BUILDDIR = build
TESTDIR = tests
OBJDIR = $(BUILDDIR)/obj
BINDIR = $(BUILDDIR)/bin

# Core source files
CORE_SOURCES = \
	rtka_u_core.c \
	rtka_core_bridge.c \
	rtka_memory.c

# Header files
HEADERS = \
	rtka_constants.h \
	rtka_types.h \
	rtka_u_core.h \
	rtka_core_bridge.h \
	rtka_memory.h

# Object files
CORE_OBJECTS = $(CORE_SOURCES:%.c=$(OBJDIR)/%.o)

# Test sources
TEST_SOURCES = \
	$(TESTDIR)/test_core.c \
	$(TESTDIR)/test_memory.c \
	$(TESTDIR)/test_integration.c

TEST_OBJECTS = $(TEST_SOURCES:%.c=$(OBJDIR)/%.o)
TEST_BINARIES = $(TEST_SOURCES:$(TESTDIR)/%.c=$(BINDIR)/%)

# Library target
LIBRTKA = $(BUILDDIR)/librtka.a

# Default target
.PHONY: all
all: $(LIBRTKA)

# Release build
.PHONY: release
release: CFLAGS = $(CFLAGS_RELEASE)
release: clean $(LIBRTKA)

# Debug build
.PHONY: debug
debug: CFLAGS = $(CFLAGS_DEBUG)
debug: clean $(LIBRTKA)

# Profile build
.PHONY: profile
profile: CFLAGS = $(CFLAGS_PROF)
profile: clean $(LIBRTKA)

# Create directories
$(OBJDIR) $(BINDIR):
	@mkdir -p $@

# Compile core objects
$(OBJDIR)/%.o: $(SRCDIR)/%.c $(HEADERS) | $(OBJDIR)
	$(CC) $(CSTD) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

# Compile test objects
$(OBJDIR)/$(TESTDIR)/%.o: $(TESTDIR)/%.c $(HEADERS) | $(OBJDIR)
	@mkdir -p $(OBJDIR)/$(TESTDIR)
	$(CC) $(CSTD) $(CFLAGS) -I$(SRCDIR) -c $< -o $@

# Build static library
$(LIBRTKA): $(CORE_OBJECTS) | $(BUILDDIR)
	ar rcs $@ $(CORE_OBJECTS)
	ranlib $@

# Build test executables
$(BINDIR)/%: $(OBJDIR)/$(TESTDIR)/%.o $(LIBRTKA) | $(BINDIR)
	$(CC) $(CSTD) $(CFLAGS) $< -L$(BUILDDIR) -lrtka -lm -lpthread -o $@

# Test targets
.PHONY: tests
tests: $(TEST_BINARIES)

.PHONY: test-core
test-core: $(BINDIR)/test_core
	@echo "Running core functionality tests..."
	./$(BINDIR)/test_core

.PHONY: test-memory
test-memory: $(BINDIR)/test_memory
	@echo "Running memory management tests..."
	./$(BINDIR)/test_memory

.PHONY: test-integration
test-integration: $(BINDIR)/test_integration
	@echo "Running integration tests..."
	./$(BINDIR)/test_integration

.PHONY: test-all
test-all: tests
	@echo "Running complete test suite..."
	@for test in $(TEST_BINARIES); do \
		echo "Running $$test..."; \
		./$$test || exit 1; \
	done
	@echo "All tests passed!"

# Valgrind memory checking
.PHONY: test-valgrind
test-valgrind: debug tests
	@echo "Running tests with Valgrind..."
	@for test in $(TEST_BINARIES); do \
		echo "Valgrind check: $$test"; \
		valgrind --leak-check=full --show-leak-kinds=all \
		         --track-origins=yes --error-exitcode=1 ./$$test; \
	done

# Static analysis
.PHONY: analyze
analyze:
	@echo "Running static analysis..."
	@which cppcheck >/dev/null 2>&1 && \
		cppcheck --enable=all --std=c11 --suppress=missingIncludeSystem $(CORE_SOURCES) || \
		echo "cppcheck not found, skipping static analysis"

# Documentation generation
.PHONY: docs
docs:
	@echo "Generating documentation..."
	@which doxygen >/dev/null 2>&1 && \
		doxygen Doxyfile || \
		echo "doxygen not found, skipping documentation generation"

# Benchmarking
$(BINDIR)/benchmark: $(OBJDIR)/benchmark.o $(LIBRTKA) | $(BINDIR)
	$(CC) $(CSTD) $(CFLAGS_RELEASE) $< -L$(BUILDDIR) -lrtka -lm -lpthread -o $@

.PHONY: benchmark
benchmark: CFLAGS = $(CFLAGS_RELEASE)
benchmark: $(BINDIR)/benchmark
	@echo "Running performance benchmarks..."
	./$(BINDIR)/benchmark

# Coverage analysis
.PHONY: coverage
coverage: CFLAGS = $(CFLAGS_DEBUG) --coverage
coverage: clean tests test-all
	@echo "Generating coverage report..."
	@which gcov >/dev/null 2>&1 && \
		gcov $(CORE_SOURCES) && \
		echo "Coverage files generated" || \
		echo "gcov not found, skipping coverage analysis"

# Installation
PREFIX ?= /usr/local
INSTALL_LIBDIR = $(PREFIX)/lib
INSTALL_INCDIR = $(PREFIX)/include/rtka

.PHONY: install
install: $(LIBRTKA)
	@echo "Installing RTKA core system..."
	install -d $(INSTALL_LIBDIR)
	install -d $(INSTALL_INCDIR)
	install -m 644 $(LIBRTKA) $(INSTALL_LIBDIR)
	install -m 644 $(HEADERS) $(INSTALL_INCDIR)

.PHONY: uninstall
uninstall:
	@echo "Uninstalling RTKA core system..."
	rm -f $(INSTALL_LIBDIR)/librtka.a
	rm -rf $(INSTALL_INCDIR)

# Cleaning
.PHONY: clean
clean:
	rm -rf $(BUILDDIR)
	rm -f *.gcov *.gcda *.gcno
	rm -f core vgcore.*

.PHONY: distclean
distclean: clean
	rm -f tags TAGS cscope.*

# Development helpers
.PHONY: tags
tags:
	@which ctags >/dev/null 2>&1 && \
		ctags -R . || \
		echo "ctags not found"

.PHONY: format
format:
	@which clang-format >/dev/null 2>&1 && \
		clang-format -i $(CORE_SOURCES) $(HEADERS) || \
		echo "clang-format not found"

# Help target
.PHONY: help
help:
	@echo "RTKA Core System Makefile"
	@echo "========================"
	@echo ""
	@echo "Build Targets:"
	@echo "  all          - Build static library (default)"
	@echo "  release      - Optimized release build"
	@echo "  debug        - Debug build with sanitizers"
	@echo "  profile      - Profile-enabled build"
	@echo ""
	@echo "Test Targets:"
	@echo "  tests        - Build all test executables"
	@echo "  test-core    - Run core functionality tests"
	@echo "  test-memory  - Run memory management tests"
	@echo "  test-integration - Run integration tests"
	@echo "  test-all     - Run complete test suite"
	@echo "  test-valgrind - Run tests with Valgrind"
	@echo ""
	@echo "Analysis Targets:"
	@echo "  analyze      - Run static analysis"
	@echo "  coverage     - Generate test coverage report"
	@echo "  benchmark    - Run performance benchmarks"
	@echo ""
	@echo "Utility Targets:"
	@echo "  install      - Install library and headers"
	@echo "  uninstall    - Remove installed files"
	@echo "  clean        - Remove build artifacts"
	@echo "  distclean    - Remove all generated files"
	@echo "  format       - Format source code"
	@echo "  tags         - Generate ctags"
	@echo "  docs         - Generate documentation"
	@echo "  help         - Show this help"

# Dependency tracking
-include $(CORE_OBJECTS:.o=.d)
-include $(TEST_OBJECTS:.o=.d)

# Generate dependency files
$(OBJDIR)/%.d: $(SRCDIR)/%.c | $(OBJDIR)
	@$(CC) $(CSTD) $(CFLAGS) -I$(SRCDIR) -MM -MT $(@:.d=.o) $< > $@

$(OBJDIR)/$(TESTDIR)/%.d: $(TESTDIR)/%.c | $(OBJDIR)
	@mkdir -p $(OBJDIR)/$(TESTDIR)
	@$(CC) $(CSTD) $(CFLAGS) -I$(SRCDIR) -MM -MT $(@:.d=.o) $< > $@

# Prevent make from deleting intermediate files
.SECONDARY:

# Mark phony targets
.PHONY: all release debug profile tests test-core test-memory test-integration \
        test-all test-valgrind analyze docs benchmark coverage install uninstall \
        clean distclean tags format help
