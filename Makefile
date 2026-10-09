CC      = cc
CFLAGS  = -std=c11 -Wall -Wextra -Wpedantic -O2 -D_NETBSD_SOURCE -D_POSIX_C_SOURCE=200809L -Iinclude -g
TARGET  = myls
OBJS    = src/options.o src/entry.o src/sort.o src/display.o src/list.o src/main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) -o $(TARGET)

src/options.o: src/options.c
	$(CC) $(CFLAGS) -c src/options.c -o src/options.o

src/entry.o: src/entry.c
	$(CC) $(CFLAGS) -c src/entry.c -o src/entry.o

src/sort.o: src/sort.c
	$(CC) $(CFLAGS) -c src/sort.c -o src/sort.o

src/display.o: src/display.c
	$(CC) $(CFLAGS) -c src/display.c -o src/display.o

src/list.o: src/list.c
	$(CC) $(CFLAGS) -c src/list.c -o src/list.o

src/main.o: src/main.c
	$(CC) $(CFLAGS) -c src/main.c -o src/main.o

test: $(TARGET)
	@sh tests/test.sh

clean:
	rm -rf src/*.o $(TARGET) tests/sandbox /tmp/sysls.* /tmp/myls.*

.PHONY: all clean test