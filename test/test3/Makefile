.ONESHELL:

temp/test3: temp/crt0.o temp/main.o
	@set -e
	@mkdir -p $(@D)
	../../bin/azld temp/crt0.o temp/main.o > temp/test3
	chmod +x temp/test3

temp/%.o: %.s
	@mkdir -p $(@D)
	gcc -c $< -o $@

.PHONY:
run: temp/test3
	$<

.PHONY:
clean:
	rm -rf temp
