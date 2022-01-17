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

test/test%.o: test/test%.s
	gcc -c $< -o $@

run: bin/azld test/test1.o
	echo test1
	$(BIN) test/test1.o > test/test1
	chmod +x test/test1
	test/test1

.PHONY: run
.SILENT: run
