#include "parse.h"
#include "container.h"
#include <elf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

ObjectFile *parse(void *head, HashTable *global_symbol_table) {
  Elf64_Ehdr *elf_header = (Elf64_Ehdr *)head;
  if (elf_header->e_phoff != 0) {
    Elf64_Phdr *program_header = (Elf64_Phdr *)(head + elf_header->e_phoff);
  }
  if (elf_header->e_shoff == 0) {
    return NULL;
  }

  ObjectFile *res = calloc(1, sizeof(ObjectFile));
  res->head = head;

  Elf64_Shdr *section_header_table = (Elf64_Shdr *)(head + elf_header->e_shoff);

  /*int shstrndx = elf_header->e_shstrndx;
  if (shstrndx == SHN_UNDEF) {
    printf("This elf file has no section str table.");
  } else if (shstrndx == SHN_XINDEX) {
    printf("This elf file's section str table is SHN_XINDEX.");
    shstrndx = section_header_table->sh_link;
  }*/

  const Elf64_Shdr *const shstr = &section_header_table[elf_header->e_shstrndx];
  fprintf(stderr, "sh: %d\n", elf_header->e_shstrndx);

  Elf64_Shdr *text_section = NULL;
  Elf64_Shdr *symtab_section_header = NULL;
  Elf64_Shdr *strtab_section_header = NULL;
  Elf64_Shdr *relatext_section_header = NULL;
  for (int i = 0; i < elf_header->e_shnum; i++) {
    // printf("section index %d\n", i);
    // printf("sh_name: %d\n", section_header_table[i].sh_name);
    const char *const section_name =
        head + shstr->sh_offset + section_header_table[i].sh_name;
    // fprintf(stderr, "sh_name: %s\n", section_name);

    // text section
    if (strncmp(section_name, ".text", strlen(".text")) == 0) {
      /*
      fprintf(stderr, "sh_type: %d\n", section_header_table[i].sh_type);
      fprintf(stderr, "sh_flags: %ld\n", section_header_table[i].sh_flags);
      fprintf(stderr, "sh_addr: %ld\n", section_header_table[i].sh_addr);
      fprintf(stderr, "sh_offset: %ld\n", section_header_table[i].sh_offset);
      fprintf(stderr, "sh_size: %ld\n", section_header_table[i].sh_size);
      fprintf(stderr, "sh_link: %d\n", section_header_table[i].sh_link);
      fprintf(stderr, "sh_info: %d\n", section_header_table[i].sh_info);
      fprintf(stderr, "sh_addralign: %ld\n",
              section_header_table[i].sh_addralign);
      fprintf(stderr, "sh_entsize: %ld\n",
              section_header_table[i].sh_entsize);
      */
      text_section = section_header_table + i;
    }

    // symtab
    if (strncmp(section_name, ".symtab", strlen(".symtab")) == 0) {
      symtab_section_header = section_header_table + i;
    }

    // strtab
    if (strncmp(section_name, ".strtab", strlen(".strtab")) == 0) {
      strtab_section_header = section_header_table + i;
    }

    // rela.text
    if (strncmp(section_name, ".rela.text", strlen(".rela.text")) == 0) {
      relatext_section_header = section_header_table + i;
    }
  }

  // analyze symtab
  {
    // fprintf(stderr, "sym:\n");
    Elf64_Sym *symtab = head + symtab_section_header->sh_offset;
    res->symtab_section_header = symtab_section_header;
    res->strtab_section_header = strtab_section_header;
    for (int i = 1; i < symtab_section_header->sh_size / sizeof(Elf64_Sym);
         i++) {
      const char *symbol_name =
          head + strtab_section_header->sh_offset + symtab[i].st_name;
      /*
      fprintf(stderr, "  st_name: %s\n", symbol_name);
      fprintf(stderr, "  st_name: %d\n", symtab[i].st_name);
      fprintf(stderr, "  st_info: %d\n", symtab[i].st_info);
      fprintf(stderr, "  st_other: %d\n", symtab[i].st_other);
      fprintf(stderr, "  st_shndx: %d\n", symtab[i].st_shndx);
      fprintf(stderr, "  st_value: %ld\n", symtab[i].st_value);
      fprintf(stderr, "  st_size: %ld\n", symtab[i].st_size);
      */

      if (ELF64_ST_BIND(symtab[i].st_info) == STB_GLOBAL) {
        hash_table_store(global_symbol_table,
                         new_string(symbol_name, strlen(symbol_name)),
                         &symtab[i]);
      }
    }
  }

  // relatext
  res->relatext_section_header = relatext_section_header;

  return res;
}
