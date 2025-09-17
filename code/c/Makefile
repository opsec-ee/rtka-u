# Makefile for RTKA-U
# Auther: H.Overman <opsec.ee@pm.me>

# OPT-000: Master Coding Standards & Static Analysis Compliance

CC = gcc
# OPT-022: C2X Feature Integration (C23 draft) + OPT-000: Comprehensive warnings
CFLAGS = -std=c2x -O2 -Wall -Wextra -Wpedantic \
         -Wconversion -Wsign-conversion -Wcast-align \
         -Wstrict-prototypes -Wmissing-prototypes \
         -Wshadow -Wundef -Wwrite-strings -Wfloat-equal \
         -Wswitch-default -Wswitch-enum \
         -march=native -flto
LDFLAGS = -lm -flto

# Object files
OBJS = rtka-u.o rtka-u_test.o

# Default target
all: test

# Compile implementation
rtka-u.o: rtka-u.c rtka-u.h
	$(CC) $(CFLAGS) -c rtka-u.c

# Compile test
rtka-u_test.o: rtka-u_test.c rtka-u_test.h rtka-u.h
	$(CC) $(CFLAGS) -c rtka-u_test.c

# Link test executable
test: $(OBJS)
	$(CC) $(OBJS) -o test $(LDFLAGS)

# Run tests
run: test
	./test

# Clean build files
clean:
	rm -f *.o test

.PHONY: all run clean

