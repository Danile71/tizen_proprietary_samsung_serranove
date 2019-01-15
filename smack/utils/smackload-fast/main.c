#include "common.h"

struct label_trie_node trie_root = {.label_id = MAX_LABELS};
struct label_trie_node **labels;
int labels_cnt, labels_alloc;

struct rule *rules;
int rules_cnt, rules_alloc;

extern void input(int argc, char *argv[]);
extern void output(void);

int main(int argc, char *argv[])
{
	input(argc, argv);
	output();
	return 0;
}
