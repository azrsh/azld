#include "elfgen.h"
#include "parse.h"
#include "util.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
  // parse argument
  if (argc != 2) {
    exit(1);
  }
  const char *filename = argv[1];

  // read object file
  const void *const head = read_binary(filename);

  // parse elf header
  const ObjectFile *const obj = parse(head);

  elfgen(head, obj);

  return 0;
}
