#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>
#include <regex.h>
#include <stdbool.h>
#include <malloc.h>
#include <search.h>

#include "parse_elf.h"

enum {
	ELF_PARSE_SUCCESS = 0,
	ELF_PARSE_NOT_ELF,
	ELF_PARSE_INVALID_CLASS,
	ELF_PARSE_MMAP_ERROR,
	ELF_PARSE_MUNMAP_ERROR,
	ELF_PARSE_NO_MEM
};

static size_t fsize(int fd)
{
	struct stat buf;
	if (fstat(fd, &buf) != 0)
		return 0;
	return buf.st_size;
}

static void *mmap_file(const char *filepath, size_t *len)
{
	int fd = open(filepath, O_RDONLY);
	if (fd < 0)
		return NULL;
	*len = fsize(fd);
	void *mem = mmap(NULL, *len, PROT_READ, MAP_PRIVATE, fd, 0);
	close(fd);
	return mem == MAP_FAILED ? NULL : mem;
}

static int munmap_file(void *elf, size_t *len)
{
	int ret = 0;

	if (munmap(elf, *len) != 0)
		ret = ELF_PARSE_MUNMAP_ERROR;

	return ret;
}

static int check_elf(const void *elf)
{
	const Elf_Ehdr *elf_header = elf;
	int ret = 0;

	if ((elf_header->e_ident[EI_MAG0] != ELFMAG0) ||
		(elf_header->e_ident[EI_MAG1] != ELFMAG1) ||
		(elf_header->e_ident[EI_MAG2] != ELFMAG2) ||
		(elf_header->e_ident[EI_MAG3] != ELFMAG3)) {
		ret = ELF_PARSE_NOT_ELF;
		goto exit;
	}

	if (elf_header->e_ident[EI_CLASS] != ELFCLASS) {
		ret = ELF_PARSE_INVALID_CLASS;
		goto exit;
	}

exit:
	return ret;
}

static Elf_Half get_machine_type(const void *elf)
{
	const Elf_Ehdr *elf_header = elf;

	return elf_header->e_machine;
}

static const char *get_plt_section_name(const void *elf)
{
	Elf_Half machine = get_machine_type(elf);
	const char *sec_name;

	switch (machine) {
	case EM_ARM:
		sec_name = ".plt";
		break;
	case EM_386:
		sec_name = ".got.plt";
		break;
	default:
		sec_name = NULL;
	}

	return sec_name;
}

static const Elf_Shdr *get_section_by_index(const void *elf, unsigned int index)
{
	const Elf_Ehdr *elf_header = elf;
	const Elf_Shdr *section_table = elf + elf_header->e_shoff;
	if (index > elf_header->e_shnum)
		return NULL;

	return section_table + index;
}

static const Elf_Shdr *get_section_by_name(const void *elf, const char* name)
{
	const Elf_Ehdr *elf_header = elf;
	const Elf_Shdr *section_table = elf + elf_header->e_shoff;
	const Elf_Shdr *string_entry = section_table + elf_header->e_shstrndx;
	const char *string_section = elf + string_entry->sh_offset;
	int i;

	for (i = 0; i != elf_header->e_shnum; ++i) {
		const Elf_Shdr *entry = section_table + i;
		if (!strcmp(name, string_section + entry->sh_name)) {
			return entry;
		}
	}
	return NULL;
}

static const Elf_Shdr *get_section_by_type(const void *elf, Elf_Word sh_type)
{
	const Elf_Ehdr *elf_header = elf;
	const Elf_Shdr *section_table = elf + elf_header->e_shoff;
	int i;

	for (i = 0; i != elf_header->e_shnum; ++i) {
		const Elf_Shdr *entry = section_table + i;
		if (entry->sh_type == sh_type)
			return entry;
	}
	return NULL;
}

static void __do_free_sym_entry(struct sym_table_entry *entry)
{
	if (entry == NULL)
		return;

	remque(entry);

	if (entry->name == NULL)
		goto name_is_free;

	free(entry->name);

name_is_free:
	free(entry);
}

static struct sym_table_entry *new_sym_entry(const char *name, Elf_Addr addr)
{
	struct sym_table_entry *elem;
	size_t len = 1;

	elem = malloc(sizeof(*elem));
	if (elem == NULL)
		return elem;

	/* There is no symbol name length limit in GCC (only memory size) */
	if (name != NULL)
		len += strlen(name);

	elem->name = malloc(len);
	if (elem->name == NULL)
		goto new_sym_entry_fail;

	if (name != NULL)
		memcpy(elem->name, name, len);

	elem->name[len - 1] = '\0';
	elem->addr = addr;
	elem->prev = NULL;
	elem->next = NULL;

	return elem;

new_sym_entry_fail:
	__do_free_sym_entry(elem);

	return NULL;
}

static struct sym_table_entry *
get_sym_addr(const void *elf, const char* table_name, const char *sym_names[],
	     size_t n, bool is_only_func)
{
	const Elf_Shdr *shdr;
	const Elf_Shdr *str_section;
	const Elf_Sym *table;
	const char *name;
	Elf_Addr addr;
	int entries_num;
	int i, j;
	struct sym_table_entry *first = NULL, *elem = NULL, *prev = NULL;
	regex_t regex;

	shdr = get_section_by_name(elf, table_name);
	if (!shdr || shdr->sh_type == SHT_NOBITS) {
		first = NULL;
		goto exit;
	}

	entries_num = shdr->sh_size / sizeof(Elf_Sym);
	if (!entries_num) {
		first = NULL;
		goto exit;
	}

	str_section = get_section_by_index(elf, ((Elf_Ehdr *)elf)->e_shstrndx);
	if (!str_section) {
		first = NULL;
		goto exit;
	}

	table = elf + shdr->sh_offset;

	str_section = get_section_by_index(elf, shdr->sh_link);
	if (!str_section) {
		first = NULL;
		goto exit;
	}

	if (sym_names) {

		for (j = 0; j < n; j++) {
			if (regcomp(&regex, sym_names[j], REG_EXTENDED))
				continue;

			for (i = 0; i < entries_num; i++) {
				name = elf + str_section->sh_offset + table[i].st_name;
				addr = table[i].st_value;

				if (is_only_func && ELF_ST_TYPE(table[i].st_info) != STT_FUNC)
					continue;

				if (!regexec(&regex, name, 0, NULL, 0)) {
					elem = new_sym_entry(name, addr);
					if (elem == NULL)
						break;
					if (first == NULL)
						first = elem;
					insque(elem, prev);
					prev = elem;
				}
			}

			regfree(&regex);
		}

	} else {
		for (i = 0; i < entries_num; i++) {
			name = elf + str_section->sh_offset + table[i].st_name;
			addr = table[i].st_value;
			elem = new_sym_entry(name, addr);
			if (elem == NULL)
				break;
			if (first == NULL)
				first = elem;
			insque(elem, prev);
			prev = elem;
		}
	}

exit:
	return first;
}

static int
get_got_plt_addrs(const void *elf, const char* entry_names[], Elf_Addr addrs[],
		  size_t n)
{
	const Elf_Shdr *shdr;
	const Elf_Sym *sym_table;
	const char* name;
	int plt_table_size;
	int i, j;
	int ret = 0;
	regex_t regex;

	shdr = get_section_by_name(elf, ".rel.plt");
	if (!shdr || shdr->sh_type == SHT_NOBITS) {
		ret = -1;
		goto exit;
	}

	plt_table_size = shdr->sh_size / sizeof(Elf_Rel);
	const Elf_Rel *plt_table = elf + shdr->sh_offset;

	const Elf_Shdr* tmp_shdr = get_section_by_index(elf, shdr->sh_link);
	if (!tmp_shdr) {
		ret = -1;
		goto exit;
	}

	sym_table = elf + tmp_shdr->sh_offset;
	tmp_shdr = get_section_by_index(elf, tmp_shdr->sh_link);
	if (!tmp_shdr) {
		ret = -1;
		goto exit;
	}

	for (j = 0; j < n; j++) {
		if (regcomp(&regex, entry_names[j], REG_EXTENDED))
				continue;

		for (i = 0; i < plt_table_size; i++) {
			name = elf + tmp_shdr->sh_offset + sym_table[ELF_R_SYM(plt_table[i].r_info)].st_name;

			if (!regexec(&regex, name, 0, NULL, 0)) {
				addrs[j] = plt_table[i].r_offset;
			}
		}

		regfree(&regex);
	}

exit:
	return ret;
}

static int __do_get_plt_addrs(const void *elf, const char* func_names[], Elf_Addr addrs[], size_t n)
{
	int ret = get_got_plt_addrs(elf, func_names, addrs, n);
	if (ret)
		goto exit;

	const char *sec_name = get_plt_section_name(elf);

	if (sec_name == NULL) {
		ret = -1;
		goto exit;
	}

	const Elf_Shdr *got_plt_shdr = get_section_by_name(elf, sec_name);
	if (!got_plt_shdr || got_plt_shdr->sh_type == SHT_NOBITS) {
		ret = -1;
		goto exit;
	}

	int base = got_plt_shdr->sh_addr - got_plt_shdr->sh_offset;

	int j;
	for (j = 0; j < n; j++) {
		if (addrs[j])
			addrs[j] = *(Elf_Addr *)(elf + addrs[j] - base) - 6;
	}

	ret = 0;

exit:
	return ret;
}

static const char *__do_get_interp(const void *elf)
{
	const Elf_Shdr *shdr = get_section_by_name(elf, ".interp");
	char *interp_str;
	size_t len = 1;

	if (shdr == NULL || shdr->sh_type == SHT_NOBITS)
		return NULL;

	len += strlen(elf + shdr->sh_offset);
	interp_str = malloc(len);
	if (interp_str == NULL)
		return NULL;

	if (len > 1)
		memcpy(interp_str, elf + shdr->sh_offset, len);
	interp_str[len - 1] = '\0';

	return interp_str;
}

static int __do_get_syms(const char *filename, const char **names, size_t n,
			 struct sym_table_entry **syms, bool is_only)
{
	size_t elf_len;
	void *elf;
	struct sym_table_entry *symtab, *dynsym;
	int ret = 0;

	elf = mmap_file(filename, &elf_len);
	if (!elf)
		return ELF_PARSE_MMAP_ERROR;

	ret = check_elf(elf);
	if (ret != 0)
		goto do_get_syms_fail;

	symtab = get_sym_addr(elf, ".symtab", names, n, is_only);
	dynsym = get_sym_addr(elf, ".dynsym", names, n, is_only);

	if (symtab != NULL) {
		if (dynsym != NULL)
			insque(dynsym, symtab);
		*syms = symtab;
	}else
		*syms = dynsym;

	ret = munmap_file(elf, &elf_len);

	return ret;

do_get_syms_fail:
	/* ignore unmap error */
	munmap_file(elf, &elf_len);

	return ret;
}


static void __attribute__((constructor)) __lib_parse_elf_ctor(void)
{
}

static void __attribute__((destructor)) __lib_parse_elf_dtor(void)
{
}



int get_interp(const char *filename, char **interp_p)
{
	const char *interp;
	size_t elf_len;
	void *elf;
	int ret = 0;

	elf = mmap_file(filename, &elf_len);
	if (!elf)
		return ELF_PARSE_MMAP_ERROR;

	ret = check_elf(elf);
	if (ret != 0)
		goto interp_fail;

	interp = __do_get_interp(elf);
	if (interp)
		*interp_p = (char *)interp;

	ret = munmap_file(elf, &elf_len);

	return ret;

interp_fail:
	/* ignore unmap error */
	munmap_file(elf, &elf_len);

	return ret;
}

int get_all_symbols(const char *filename, struct sym_table_entry **syms)
{
	return __do_get_syms(filename, NULL, 0, syms, false);
}

int get_symbols_by_names(const char *filename, const char **names, size_t n, struct sym_table_entry **syms, bool is_only)
{
	return __do_get_syms(filename, names, n, syms, is_only);
}

int get_plt_addrs(const char *filename, const char **names, size_t n, Elf_Addr **addrs_p)
{
	size_t elf_len;
	void *elf;
	Elf_Addr *addrs;
	int ret = 0;

	elf = mmap_file(filename, &elf_len);
	if (!elf)
		return ELF_PARSE_MMAP_ERROR;

	ret = check_elf(elf);
	if (ret != 0)
		goto get_plt_fail;

	addrs = (Elf_Addr *)calloc(n, sizeof(Elf_Addr));
	if (addrs == NULL) {
		ret = ELF_PARSE_NO_MEM;
		goto get_plt_fail;
	}

	if (!__do_get_plt_addrs(elf, names, addrs, n))
		*addrs_p = addrs;

	ret = munmap_file(elf, &elf_len);

	if (ret == ELF_PARSE_MUNMAP_ERROR) {
		free(addrs);
		*addrs_p = NULL;
	}

	return ret;

get_plt_fail:
	/* ignore unmap error */
	munmap_file(elf, &elf_len);

	return ret;
}

void free_sym_entry(struct sym_table_entry *entry)
{
	__do_free_sym_entry(entry);
}

const char *get_str_error(int err)
{
	char *str;

	switch (err) {
	case ELF_PARSE_SUCCESS:
		str = "Success";
		break;
	case ELF_PARSE_NOT_ELF:
		str = "File is not ELF object file";
		break;
	case ELF_PARSE_INVALID_CLASS:
		str = "Invalid ELF class";
		break;
	case ELF_PARSE_MMAP_ERROR:
		str = "Cannot mmap file";
		break;
	case ELF_PARSE_MUNMAP_ERROR:
		str = "Cannot mumap file";
		break;
	case ELF_PARSE_NO_MEM:
		str = "No memory";
		break;
	default:
		str = "Unknown error";
	}

	return str;
}
