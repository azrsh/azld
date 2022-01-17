.ONESHELL:

SRCS=$(wildcard src/*.c)

CC:=gcc

CFLAGS:=-std=c11 -g -Wall -Wextra
OBJS=$(SRCS:src/%.c=bin/%.o)
BIN:=bin/azld

$(BIN): $(OBJS)
	@mkdir -p $(@D)
	$(CC) -o $@ $^ $(LDFLAGS)

bin/%.o: src/%.c src/*.h
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c -o $@ $<

test.o: test.s
	gcc -c test.s

run: bin/azld test.o
	echo test.o 
	$(BIN) test.o > hello
	./hello

.PHONY: run
.SILENT: run
