#include "elfgen.h"
#include "container.h"
#include "util.h"
#include <elf.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define LOAD_ADDRESS 0x41000

void write_elf_header(void *buffer, size_t *len) {
  Elf64_Ehdr ehdr = {
      .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB,
                  EV_CURRENT, ELFOSABI_SYSV},
      .e_type = ET_EXEC,
      .e_machine = EM_X86_64,
      .e_version = EV_CURRENT,
      .e_entry = LOAD_ADDRESS + sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr),
      .e_phoff = sizeof(Elf64_Ehdr),
      .e_shoff = 0,
      .e_flags = 0x0,
      .e_ehsize = sizeof(Elf64_Ehdr),
      .e_phentsize = sizeof(Elf64_Phdr),
      .e_phnum = 1,
      .e_shentsize = 0,
      .e_shnum = 0,
      .e_shstrndx = 0,
  };

  memcpy(buffer + *len, &ehdr, sizeof(Elf64_Ehdr));
  *len += sizeof(Elf64_Ehdr);
}

void write_program_header(void *buffer, size_t *len, size_t text_len) {
  Elf64_Phdr phdr = {
      .p_type = PT_LOAD,
      .p_offset = 0,
      .p_vaddr = LOAD_ADDRESS,
      .p_paddr = 0,
      .p_filesz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + text_len,
      .p_memsz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + text_len,
      .p_flags = PF_R | PF_X,
      .p_align = 0x1000,
  };

  memcpy(buffer + *len, &phdr, sizeof(Elf64_Phdr));
  *len += sizeof(Elf64_Phdr);
}

void elfgen(Vector *objs, HashTable /*Elf64_Sym*/ *global_symbol_table) {
  ObjectFile *obj = vector_get(objs, 0);
  void *head = obj->head;

  Elf64_Ehdr *elf_header = (Elf64_Ehdr *)head;
  if (elf_header->e_shoff == 0) {
    ERROR("No section header");
  }

  const Elf64_Shdr *const section_header_table =
      (Elf64_Shdr *)(head + elf_header->e_shoff);

  const Elf64_Shdr *const shstr = &section_header_table[elf_header->e_shstrndx];

  const Elf64_Shdr *text_section_header = NULL;
  for (int i = 0; i < elf_header->e_shnum; i++) {
    const char *const section_name =
        head + shstr->sh_offset + section_header_table[i].sh_name;
    if (strncmp(section_name, ".text", strlen(".text")) == 0) {
      text_section_header = &section_header_table[i];
    }
  }

  const Elf64_Shdr *data_section_header = NULL;
  for (int i = 0; i < elf_header->e_shnum; i++) {
    const char *const section_name =
        head + shstr->sh_offset + section_header_table[i].sh_name;
    if (strncmp(section_name, ".data", strlen(".data")) == 0) {
      data_section_header = &section_header_table[i];
    }
  }

  const size_t text_len = text_section_header->sh_size;
  const size_t data_len = data_section_header->sh_size;

  void *buffer = malloc(1024 * 1024);
  size_t buffer_len = 0;

  write_elf_header(buffer, &buffer_len);
  write_program_header(buffer, &buffer_len, text_len);

  size_t buffer_text_section_offset = buffer_len;
  {
    const void *text_section = head + text_section_header->sh_offset;
    memcpy(buffer + buffer_len, text_section, text_len);
    buffer_len += text_len;
  }

  size_t buffer_data_section_offset = buffer_len;
  {
    const void *data_section = head + data_section_header->sh_offset;
    memcpy(buffer + buffer_len, data_section, data_len);
    buffer_len += data_len;
  }

  // symbol resolve
  {
    const Elf64_Sym *const symtab =
        head + obj->symtab_section_header->sh_offset;
    for (int i = 1; i < obj->symtab_section_header->sh_size / sizeof(Elf64_Sym);
         i++) {
      const char *const symbol_name =
          head + obj->strtab_section_header->sh_offset + symtab[i].st_name;
      fprintf(stderr, "analyze: %s\n", symbol_name);
    }

    // Soymbol resolve
    // x86_64ではRelaのみを用いるのでそれだけを考えればよい
    const Elf64_Rela *const relatext =
        head + obj->relatext_section_header->sh_offset;
    for (int i = 0;
         i < obj->relatext_section_header->sh_size / sizeof(Elf64_Rela); i++) {
      int r_sym = ELF64_R_SYM(relatext[i].r_info);
      Elf64_Word r_type = ELF64_R_TYPE(relatext[i].r_info);

      const Elf64_Sym symbol = symtab[r_sym];

      const char *const symbol_name =
          head + obj->strtab_section_header->sh_offset + symbol.st_name;
      fprintf(stderr, "resolve: %s\n", symbol_name);

      int st_bind = ELF64_ST_BIND(symbol.st_info);
      int st_type = ELF64_ST_TYPE(symbol.st_info);

      // TODO: Symbolごとに事前に計算してキャッシュできるはず
      // 入力されたファイル上のシンボルのアドレスを出力する実行可能ファイルの仮想アドレス上の位置に変換する
      size_t symbol_section_offset = 0;
      if (&section_header_table[symbol.st_shndx] == text_section_header) {
        symbol_section_offset = buffer_text_section_offset;
      } else if (&section_header_table[symbol.st_shndx] ==
                 data_section_header) {
        symbol_section_offset = buffer_data_section_offset;
      } else if (symbol.st_shndx == SHN_UNDEF) {
        ERROR("Undefined section.");
      } else {
        ERROR("Unreachable: unknown section.");
      }

      Elf64_Addr symbol_value;
      size_t target_offset = buffer_text_section_offset + relatext[i].r_offset;
      Elf64_Addr *target = buffer + target_offset;
      fprintf(stderr, "r_offset: %ld\n", relatext[i].r_offset);
      if (r_type == R_X86_64_32S) {
        fprintf(stderr, "r_type: %s\n", "R_X86_64_32S");
        // S + A
        symbol_value = *target + (LOAD_ADDRESS + symbol_section_offset +
                                  relatext[i].r_addend);
      } else if (r_type == R_X86_64_PLT32) {
        fprintf(stderr, "r_type: %s\n", "R_X86_64_PLT32");
        fprintf(stderr, "r_addend: %ld\n", relatext[i].r_addend);
        // L + A - P
        symbol_value = symbol.st_value + relatext[i].r_addend -
                       relatext[i].r_offset + *target;
        // symbol_value = LOAD_ADDRESS + symbol_section_offset +
        // symbol.st_value;
        //  symbol_value = 32;
      } else {
        ERROR("Unreachable: unknown r_type");
      }
      *target = symbol_value;
    }
  }

  write(1, buffer, buffer_len);
}
