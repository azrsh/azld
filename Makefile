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

test/temp/test%.o: test/test%.s
	gcc -c $< -o $@

test: bin/azld test/temp/test1.o test/temp/test2.o
	echo test1
	$(BIN) test/temp/test1.o > test/temp/test1
	chmod +x test/temp/test1
	test/temp/test1
	echo test2
	$(BIN) test/temp/test2.o > test/temp/test2
	chmod +x test/temp/test2
	test/temp/test2
	echo test3
	cd test/test3
	$(MAKE) clean run

.PHONY: test
.SILENT: test
