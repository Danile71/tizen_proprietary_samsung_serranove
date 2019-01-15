#ifndef _PARSE_ELF_
#define _PARSE_ELF_

#ifdef __cplusplus
extern "C" {
#endif

#include <elf.h>

#define SIZEOF_VOID_P 4
#if SIZEOF_VOID_P == 8
typedef Elf64_Half Elf_Half;
typedef Elf64_Ehdr Elf_Ehdr;
typedef Elf64_Shdr Elf_Shdr;
typedef Elf64_Sym  Elf_Sym;
typedef Elf64_Addr Elf_Addr;
typedef Elf64_Word Elf_Word;
typedef Elf64_Rel Elf_Rel;
#define ELFCLASS ELFCLASS64
#define ELF_R_SYM(x) ELF64_R_SYM(x)
#define ELF_ST_TYPE(x) ELF64_ST_TYPE(x)
#elif SIZEOF_VOID_P == 4
typedef Elf32_Half Elf_Half;
typedef Elf32_Ehdr Elf_Ehdr;
typedef Elf32_Shdr Elf_Shdr;
typedef Elf32_Sym  Elf_Sym;
typedef Elf32_Addr Elf_Addr;
typedef Elf32_Word Elf_Word;
typedef Elf32_Rel Elf_Rel;
#define ELFCLASS ELFCLASS32
#define ELF_R_SYM(x) ELF32_R_SYM(x)
#define ELF_ST_TYPE(x) ELF32_ST_TYPE(x)
#else
#error "Unknown void* size"
#endif


struct sym_table_entry {
	struct sym_table_entry *next;
	struct sym_table_entry *prev;
	char* name;
	Elf_Addr addr;
};

int get_interp(const char *filename, char **interp_p);
int get_all_symbols(const char *filename, struct sym_table_entry **syms);
int get_symbols_by_names(const char *filename, const char **names, size_t n, struct sym_table_entry **syms, bool is_only);
int get_plt_addrs(const char *filename, const char **names, size_t n, Elf_Addr **addrs_p);

void free_sym_entry(struct sym_table_entry *entry);

const char *get_str_error(int err);

#ifdef __cplusplus
}
#endif

#endif /* _PARSE_ELF_ */
