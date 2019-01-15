#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <err.h>
#include <fts.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include "common.h"

#define SMACK_RULES_DIR  "/etc/smack/accesses.d"

#define parse_error(...) {					\
	fprintf(stderr, "Parse error in file %s:%d: ",		\
		curr_file, curr_line);				\
	fprintf(stderr, __VA_ARGS__);				\
	fprintf(stderr, "\n");					\
	return -1;						\
}

#define next_char() ((curr_buf_pos < curr_buf_size) ? curr_buf[curr_buf_pos++] : '\0')

static const char *curr_file;
static char *curr_buf;
static int curr_buf_pos, curr_buf_size;
static int curr_line;


static inline void skip_error_lines(void)
{
    if ( curr_buf[curr_buf_pos-1] != '\n') {
        while (curr_buf_pos < curr_buf_size && curr_buf[curr_buf_pos] != '\n') {
            ++curr_buf_pos;
        }
    }
}

static inline void skip_empty_lines(void)
{
	while (curr_buf_pos < curr_buf_size && curr_buf[curr_buf_pos] == '\n') {
		++curr_buf_pos;
		++curr_line;
	}
}

static int parse_label()
{
	struct label_trie_node *node = &trie_root;
	short len = 0;
	char label[SMACK_LABEL_LEN + 1];
	int c;

	do {
		c = next_char();
	} while (c == ' ');

	if (c == '-')
		parse_error("Smack label cannot begin with '-'");

	for (; c != ' '; c = next_char()) {
		switch (c) {
		case '~':
		case '/':
		case '"':
		case '\\':
		case '\'':
			parse_error("Character %c is prohibited in Smack label", c);
		case '\t':
			parse_error("Unexpected white space");
		case '\n':
			parse_error("Unexpected EOL");
		case '\0':
			parse_error("Unexpected EOF");
		}

		label[len++] = c;
		if (len > SMACK_LABEL_LEN)
			parse_error("Label too long");

		if (node->children[c] == NULL) {
			node->children[c] = calloc(1, sizeof(struct label_trie_node));
			if (node->children[c] == NULL)
				errx(EXIT_FAILURE, "Unable to allocate %zd bytes of memory", sizeof(struct label_trie_node));

			node->children[c]->parent = node;
			node->children[c]->label_id = MAX_LABELS;
		}

		node = node->children[c];
	}

	if (node->label_id != MAX_LABELS)
		return node->label_id;

	if (labels_cnt == MAX_LABELS - 1)
		errx(EXIT_FAILURE, "Maximum number of labels (%d) exceeded", MAX_LABELS - 1);

	buf_auto_resize(labels, labels_cnt, labels_alloc);

	label[len] = '\0';
	node->label = strdup(label);
	labels[labels_cnt] = node;
	node->label_id = labels_cnt++;

	return node->label_id;
}

static int parse_access()
{
	int c;
	int len = 0;
	short access = 0;

	do {
		c = next_char();
	} while (c == ' ');

	while (1) {
		switch (c) {
		case 'r':
		case 'R':
			access |= MODE_READ;
			break;
		case 'w':
		case 'W':
			access |= MODE_WRITE;
			break;
		case 'x':
		case 'X':
			access |= MODE_EXEC;
			break;
		case 'a':
		case 'A':
			access |= MODE_APPEND;
			break;
		case 't':
		case 'T':
			access |= MODE_TRANS;
			break;
		case 'l':
		case 'L':
			access |= MODE_LOCK;
			break;
		case '-':
			break;
		case ' ':
		case '\n':
		case '\0':
			if (len > 0)
				return access;
			else
				parse_error("Empty mode string");
		default:
			parse_error("Invalid character '%c' in mode string", c);
		}

		++len;
		c = next_char();
	}

	return access;
}

static inline void parse_file(const char *path)
{
	int fd = open(path, O_RDONLY | O_NOATIME, 0);

	if (fd < 0) {
		warn("Unable to open file %s", path);
		return;
	}

	curr_buf_size = lseek(fd, 0, SEEK_END);
	if (curr_buf_size < 0) {
		warn("Unable to read file %s", path);
		close(fd);
		return;
	}

	if (curr_buf_size == 0) {
		close(fd);
		return;
	}

	curr_buf = mmap(NULL, curr_buf_size, PROT_READ, MAP_PRIVATE | MAP_POPULATE, fd, 0);
	if (curr_buf == MAP_FAILED) {
		warn("Unable to read file %s", path);
		close(fd);
		return;
	}

	curr_file = path;
	curr_buf_pos = 0;
	curr_line = 0;

	skip_empty_lines();
	while (curr_buf_pos < curr_buf_size) {
		int ret = 0;
		buf_auto_resize(rules, rules_cnt, rules_alloc);
		++curr_line;

		if (ret >= 0 && (ret = parse_label()) >= 0)
			RULE_SUBJ(rules[rules_cnt]) = ret;

		if (ret >= 0 && (ret = parse_label()) >= 0)
			RULE_OBJ(rules[rules_cnt]) = ret;

		if (ret >= 0 && (ret = parse_access()) >= 0)
			rules[rules_cnt].access1 = ret;

		if (ret >= 0)
		{
			if (curr_buf_pos == curr_buf_size || curr_buf[curr_buf_pos - 1] == '\n') {
				rules[rules_cnt].access2 = -1;
				skip_empty_lines();
				++rules_cnt;
			}
			else if ((ret = parse_access()) >= 0) {
				rules[rules_cnt].access2 = ret;
				skip_empty_lines();
				++rules_cnt;
			}
		}

		if (ret < 0)
		{
			skip_error_lines();
		}
	}

	close(fd);
}

void input(int argc, char *argv[])
{
	const char* path_argv[argc + 1];
	FTS *fts;
	FTSENT *ftsent;
	int i;

	if (argc > 1) {
		for (i = 1; i < argc; ++i)
			path_argv[i - 1] = argv[i];
		path_argv[argc - 1] = NULL;
	} else {
		path_argv[0] = SMACK_RULES_DIR;
		path_argv[1] = NULL;
	}

	fts = fts_open((char * const *) path_argv, FTS_PHYSICAL | FTS_NOSTAT, NULL);
	if (fts == NULL)
		err(EXIT_FAILURE, "Unable to open input path");

	while ((ftsent = fts_read(fts)) != NULL) {
		if (ftsent->fts_info == FTS_NSOK)
			parse_file(ftsent->fts_accpath);
	}

	fts_close(fts);
}
