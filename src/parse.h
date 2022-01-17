#include <elf.h>

#ifndef PARSE_H
#define PARSE_H

typedef struct ObjectFile {
  // Elf64_Ehdr *elf_header;
  // Elf64_Shdr *section_header_table;
  // Elf64_Shdr *shstr_header;
  // Elf64_Shdr *text_section_header;
  // Elf64_Shdr *data_section_header;

  Elf64_Shdr *symtab_section_header;
  Elf64_Shdr *strtab_section_header;
  Elf64_Shdr *relatext_section_header;
} ObjectFile;

ObjectFile *parse(const void *head);

#endif
