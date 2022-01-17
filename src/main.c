#include "container.h"
#include "elfgen.h"
#include "parse.h"
#include "util.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
  // parse argument
  if (argc < 2) {
    ERROR("No input file specified.");
  }

  HashTable *global_symbol_table = new_hash_table();
  Vector *objs = new_vector(16);
  for (int i = 1; i < argc; i++) {
    const char *filename = argv[i];

    // read object file
    void *head = read_binary(filename);

    // parse elf header
    ObjectFile *obj = parse(head, global_symbol_table);

    vector_push_back(objs, obj);
  }

  elfgen(objs, global_symbol_table);

  return 0;
}
