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

run: bin/azld
	echo test.o 
	$(BIN) test.o
	echo test 
	$(BIN) test

.PHONY: run
.SILENT: run
