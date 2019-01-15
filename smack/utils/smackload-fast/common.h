#include <stdint.h>
#include <limits.h>

#define MODE_READ       0x01
#define MODE_WRITE      0x02
#define MODE_EXEC       0x04
#define MODE_APPEND     0x08
#define MODE_TRANS      0x10
#define MODE_LOCK       0x20
#define MODE_MAX        0x40

#define SMACK_LABEL_LEN 255
#define MAX_LABELS      USHRT_MAX

#define buf_auto_resize(ptr, cnt, alloc) {				\
	if ((cnt) >= (alloc)) {						\
		__typeof__(ptr) ptr_new;				\
		(alloc) = ((alloc) == 0) ? 256 : ((alloc) << 1);	\
		ptr_new = realloc((ptr), (alloc) * sizeof(*ptr));	\
		if (ptr_new == NULL)					\
			errx(EXIT_FAILURE,				\
				"Unable to allocate %zd bytes memory",	\
				(alloc) * sizeof(*ptr));		\
		(ptr) = ptr_new;					\
	}								\
}

struct label_trie_node {
	uint16_t label_id;
	const char *label;
	struct label_trie_node *parent;
	struct label_trie_node *children[256];
};

struct rule {
	union {
		uint32_t subjobj;
		struct {
			uint32_t obj:16;
			uint32_t subj:16;
		} s;
	} u;
	int8_t access1;
	int8_t access2;
};

#define RULE_SUBJ(R)    ((R).u.s.subj)
#define RULE_OBJ(R)     ((R).u.s.obj)
#define RULE_SUBJOBJ(R) ((R).u.subjobj)

extern struct label_trie_node trie_root;
extern struct label_trie_node **labels;
extern int labels_cnt, labels_alloc;

extern struct rule *rules;
extern int rules_cnt, rules_alloc;
