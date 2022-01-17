#include "container.h"
#include "parse.h"

#ifndef ELFGEN_H
#define ELFGEN_H

void elfgen(Vector /*ObjectFile*/ *objs,
            HashTable /*Elf64_Sym*/ *global_symbol_table);

#endif
