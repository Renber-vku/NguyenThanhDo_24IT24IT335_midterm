CC      ?= cc
CFLAGS  ?= -std=c11 -Wall -Wextra -Wpedantic -O2 -D_NETBSD_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -g
TARGET  := myls
SRCS    := src/options.c src/entry.c src/sort.c src/display.c src/list.c src/main.c
OBJS    := $(SRCS:src/%.c=build/%.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS)

build/%.o: src/%.c
	@mkdir -p build
	$(CC) $(CFLAGS) -c $< -o $@

test: $(TARGET)
	@sh tests/test.sh

clean:
	rm -rf build $(TARGET) tests/sandbox

.PHONY: all clean test