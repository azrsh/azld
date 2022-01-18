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

void write_program_header(void *buffer, size_t *len, size_t code_len) {
  Elf64_Phdr phdr = {
      .p_type = PT_LOAD,
      .p_offset = 0,
      .p_vaddr = LOAD_ADDRESS,
      .p_paddr = 0,
      .p_filesz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + code_len,
      .p_memsz = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) + code_len,
      .p_flags = PF_R | PF_X,
      .p_align = 0x1000,
  };

  memcpy(buffer + *len, &phdr, sizeof(Elf64_Phdr));
  *len += sizeof(Elf64_Phdr);
}

void elfgen(Vector *objs, HashTable /*Elf64_Sym*/ *global_symbol_table) {
  void *buffer = malloc(1024 * 1024);
  size_t buffer_len = 0;

  size_t text_len_sum = 0;
  size_t data_len_sum = 0;
  for (int i = 0; i < vector_length(objs); i++) {
    ObjectFile *obj = vector_get(objs, i);
    text_len_sum += obj->text_section_header->sh_size;
    data_len_sum += obj->data_section_header->sh_size;
  }

  // write header
  write_elf_header(buffer, &buffer_len);
  write_program_header(buffer, &buffer_len, text_len_sum + data_len_sum);

  // write text and data section
  size_t buffer_text_section_offsets[vector_length(objs)];
  size_t buffer_data_section_offsets[vector_length(objs)];
  for (int i = 0; i < vector_length(objs); i++) {
    ObjectFile *obj = vector_get(objs, i);
    void *head = obj->head;

    const Elf64_Shdr *const text_section_header = obj->text_section_header;
    const Elf64_Shdr *const data_section_header = obj->data_section_header;
    const size_t text_len = obj->text_section_header->sh_size;
    const size_t data_len = obj->data_section_header->sh_size;

    buffer_text_section_offsets[i] = buffer_len;
    {
      const void *text_section = head + text_section_header->sh_offset;
      memcpy(buffer + buffer_len, text_section, text_len);
      buffer_len += text_len;
    }

    buffer_data_section_offsets[i] = buffer_len;
    {
      const void *data_section = head + data_section_header->sh_offset;
      memcpy(buffer + buffer_len, data_section, data_len);
      buffer_len += data_len;
    }
  }

  // symbol resolve
  for (int obj_index = 0; obj_index < vector_length(objs); obj_index++) {
    ObjectFile *obj = vector_get(objs, obj_index);
    void *head = obj->head;

    const Elf64_Shdr *const section_header_table = obj->section_header_table;
    const Elf64_Shdr *text_section_header = obj->text_section_header;
    const Elf64_Shdr *data_section_header = obj->data_section_header;

    const Elf64_Sym *const symtab =
        head + obj->symtab_section_header->sh_offset;

    // Soymbol resolve
    // x86_64ではRelaのみを用いるのでそれだけを考えればよい
    const Elf64_Rela *const relatext =
        head + obj->relatext_section_header->sh_offset;
    for (int relatext_index = 0;
         relatext_index <
         obj->relatext_section_header->sh_size / sizeof(Elf64_Rela);
         relatext_index++) {
      int r_sym = ELF64_R_SYM(relatext[relatext_index].r_info);
      Elf64_Word r_type = ELF64_R_TYPE(relatext[relatext_index].r_info);

      Elf64_Sym symbol = symtab[r_sym];

      const char *const symbol_name = obj->strtab + symbol.st_name;
      fprintf(stderr, "resolve: %s\n", symbol_name);

      int st_bind = ELF64_ST_BIND(symbol.st_info);
      int st_type = ELF64_ST_TYPE(symbol.st_info);

      // TODO: Symbolごとに事前に計算してキャッシュできるはず
      // 入力されたファイル上のシンボルのアドレスを出力する実行可能ファイルの仮想アドレス上の位置に変換する
      size_t symbol_section_offset = 0;
      if (&section_header_table[symbol.st_shndx] == text_section_header) {
        symbol_section_offset = buffer_text_section_offsets[obj_index];
      } else if (&section_header_table[symbol.st_shndx] ==
                 data_section_header) {
        symbol_section_offset = buffer_data_section_offsets[obj_index];
      } else if (symbol.st_shndx == SHN_UNDEF) {

        //オブジェクトファイル内にないシンボルを他のオブジェクトファイルで探索する
        //後でparse.cで構築したglobal_symbol_tableを使用するように変更する;
        int search_obj_index;
        int search_symbol_index;
        for (search_obj_index = 0; search_obj_index < vector_length(objs);
             search_obj_index++) {
          if (obj_index == search_obj_index) {
            continue;
          }

          ObjectFile *search_obj = vector_get(objs, search_obj_index);
          const size_t search_obj_symtab_len =
              search_obj->symtab_section_header->sh_size / sizeof(Elf64_Sym);
          for (search_symbol_index = 0;
               search_symbol_index < search_obj_symtab_len;
               search_symbol_index++) {
            const Elf64_Sym *const search_symtab = search_obj->symtab;
            const char *const search_symbol_name =
                search_obj->strtab + search_symtab[search_symbol_index].st_name;
            if (search_symtab[search_symbol_index].st_shndx != SHN_UNDEF &&
                strncmp(symbol_name, search_symbol_name, strlen(symbol_name)) ==
                    0) {
              break;
            }
          }
          if (search_symbol_index < search_obj_symtab_len) {
            break;
          }
        }
        if (search_obj_index < vector_length(objs)) {
          // TODO: dataセクションだった場合に解決できない解決できない
          symbol_section_offset = buffer_text_section_offsets[search_obj_index];

          ObjectFile *search_obj = vector_get(objs, search_obj_index);
          const Elf64_Sym *const search_symtab = search_obj->symtab;
          symbol = search_symtab[search_symbol_index];
        } else {
          ERROR("Undefined symbol: %s", symbol_name);
        }

      } else {
        ERROR("Unreachable: unknown section.");
      }

      Elf64_Addr symbol_value;
      size_t target_offset = buffer_text_section_offsets[obj_index] +
                             relatext[relatext_index].r_offset;
      Elf64_Addr *target = buffer + target_offset;
      if (r_type == R_X86_64_32S) {
        fprintf(stderr, "r_type: %s\n", "R_X86_64_32S");
        // S + A
        symbol_value = *target + (LOAD_ADDRESS + symbol_section_offset +
                                  relatext[relatext_index].r_addend);
      } else if (r_type == R_X86_64_PLT32) {
        fprintf(stderr, "r_type: %s\n", "R_X86_64_PLT32");
        fprintf(stderr, "target_section_offset: 0x%lx\n",
                buffer_text_section_offsets[obj_index]);
        fprintf(stderr, "symbol_section_offset: 0x%lx\n",
                symbol_section_offset);
        fprintf(stderr, "st_value: 0x%lx\n", symbol.st_value);
        fprintf(stderr, "r_addend: 0x%lx\n", relatext[relatext_index].r_addend);
        fprintf(stderr, "r_offset: 0x%lx\n", relatext[relatext_index].r_offset);
        fprintf(stderr, "*target: 0x%lx\n", *target);
        // L + A - P
        symbol_value = (symbol_section_offset + symbol.st_value + *target) +
                       relatext[relatext_index].r_addend - target_offset;
        fprintf(stderr, "symbol_value: 0x%lx\n", symbol_value);
      } else {
        ERROR("Unreachable: unknown r_type");
      }
      *target = symbol_value;
    }
  }

  write(1, buffer, buffer_len);
}
