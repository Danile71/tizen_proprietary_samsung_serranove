#include <stdio.h>
#include <stdbool.h>
#include <malloc.h>
#include <search.h>
#include <string.h>

#include "parse_elf.h"


unsigned long get_addr_by_name(struct sym_table_entry *entry, const char *name)
{
	struct sym_table_entry *first;
	size_t len_one, len_two, len;

	first = entry;

	do {
		len_one = strlen(entry->name);
		len_two = strlen(name);
		len = len_one > len_two ? len_two : len_one;
		if (strncmp(entry->name, name, len) == 0)
			return entry->addr;
		entry = entry->next;
	} while (entry != NULL);

	return 0;
}

void print_sym_table(struct sym_table_entry *entry)
{
	struct sym_table_entry *first;
	int i;

	if (!entry)
		return;

	first = entry;

	do {
		printf("%08x %s\n", entry->addr, entry->name);
		entry = entry->next;
	} while (entry != NULL);
}

void print_sym_addrs(struct sym_table_entry *entry, const char **names,
		     size_t len)
{
	int i;
	int addr;

	if (!entry)
		return;

	for (i = 0; i < len; i++)
		printf("%08x\n", get_addr_by_name(entry, names[i]));
}

void clean_syms(struct sym_table_entry *entry)
{
	struct sym_table_entry *next = NULL;

	do {
		next = entry->next;
		free_sym_entry(entry);
		entry = next;
	} while (entry != NULL);
}

void print_usage(const char* pr_path)
{
	fprintf(stderr,
		"usage: %s path_to_elf %s\n",
		pr_path,
		"[ -i | -sa | -s sym_name1 [sym_name2 ...] | -r rel_name1 [rel_name2 ...]]"
	);
}

int main(int argc, char **argv)
{
	const char *filename = argv[1];
	size_t entries_num = 0;
	int i;
	int ret = 0;

	/* TODO fix coding style */
	if (argc >= 3) {
		if (argc == 3) {
			if (!strcmp(argv[2], "-i")) {
				char *interp = NULL;

				ret = get_interp(filename, &interp);
				if (ret != 0) {
					printf("Error: %s\n", get_str_error(ret));
					goto fail_exit;
				}
				if (interp)
					printf("%s\n", interp);

			} else if (!strcmp(argv[2], "-sa")) {
				struct sym_table_entry *syms = NULL;

				ret = get_all_symbols(filename, &syms);
				if (ret != 0) {
					printf("Error: %s\n", get_str_error(ret));
					goto fail_exit;
				}
				print_sym_table(syms);
				clean_syms(syms);
			} else {
				ret = -1;
				goto print_usage_exit;
			}

		} else if (argc > 3) {
			const char **names = (const char **)&argv[3];
			entries_num = argc - 3;

			if (!strcmp(argv[2], "-s") || !strcmp(argv[2], "-sf")) {
				bool is_only_func = argv[2][2] == 'f';
				struct sym_table_entry *syms = NULL;

				ret = get_symbols_by_names(filename, names, entries_num, &syms, is_only_func);
				if (ret != 0) {
					printf("Error: %s\n", get_str_error(ret));
					goto fail_exit;
				}
				print_sym_addrs(syms, names, entries_num);
				clean_syms(syms);

			} else if (!strcmp(argv[2], "-r")) {
				Elf_Addr *addrs = NULL;

				ret = get_plt_addrs(filename, names, entries_num, &addrs);
				if (ret != 0) {
					printf("Error: %s\n", get_str_error(ret));
					ret = -1;
				}
				for (i = 0; i < entries_num; i++)
					printf("%08x\n", addrs[i]);
				free(addrs);
			} else {
				ret = -1;
				goto print_usage_exit;
			}
		}
	} else {
		ret = -1;
		goto print_usage_exit;
	}

	return ret;

print_usage_exit:
	print_usage(argv[0]);

fail_exit:
	return ret;
}
