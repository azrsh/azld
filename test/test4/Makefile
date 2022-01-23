.ONESHELL:

temp/bin: temp/crt0.o temp/main.o temp/data.o
	@set -e
	@mkdir -p $(@D)
	../../bin/azld temp/crt0.o temp/main.o temp/data.o > temp/bin
	chmod +x temp/bin

temp/%.o: %.s
	@mkdir -p $(@D)
	gcc -c $< -o $@

.PHONY:
run: temp/bin
	$<

.PHONY:
clean:
	rm -rf temp
