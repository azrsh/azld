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

test: bin/azld test/test1.o test/test2.o
	echo test1
	$(BIN) test/test1.o > test/test1
	chmod +x test/test1
	test/test1
	echo test2
	$(BIN) test/test2.o > test/test2
	chmod +x test/test2
	test/test2

.PHONY: test
.SILENT: test
