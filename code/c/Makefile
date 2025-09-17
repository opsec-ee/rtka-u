# Makefile for RTKA-U
# Auther: H.Overman <opsec.ee@pm.me>

CC = gcc
CFLAGS = -O2 -Wall -Wextra -std=c11
LDFLAGS = -lm

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
