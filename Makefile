CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -D_NETBSD_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -g
TARGET  := myls
OBJS    := src/options.o src/entry.o src/sort.o src/display.o src/list.o src/main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

.c.o:
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	@sh tests/test.sh

clean:
	rm -rf src/*.o $(TARGET) tests/sandbox /tmp/sysls.* /tmp/myls.*

.PHONY: all clean test