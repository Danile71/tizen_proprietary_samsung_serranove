#include <stdlib.h>
#include <err.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include "common.h"
#include "modes.h"

#define SMACK_LOAD2_PATH "/smack/load2"

static char *out_buf;
static int out_buf_pos;

int compare_rules(const void *p1, const void *p2)
{
	struct rule *rule1 = (struct rule *) p1;
	struct rule *rule2 = (struct rule *) p2;

	return RULE_SUBJOBJ(*rule1) - RULE_SUBJOBJ(*rule2);
}

static inline void print_rule(const char *subj, const char *obj, const char *mode)
{
	const char *c;

	for (c = subj; *c; ++c)
		out_buf[out_buf_pos++] = *c;
	out_buf[out_buf_pos++] = ' ';

	for (c = obj; *c; ++c)
		out_buf[out_buf_pos++] = *c;
	out_buf[out_buf_pos++] = ' ';

	for (c = mode; *c; ++c)
		out_buf[out_buf_pos++] = *c;
	out_buf[out_buf_pos++] = '\n';
}

static inline void print_rules(int fd)
{
	struct rule rule;
	int access = 0;
	int i;
	int ret;

	RULE_SUBJOBJ(rule) = 0;
	out_buf = malloc(rules_cnt * (SMACK_LABEL_LEN * 2 + 16));
	if (out_buf == NULL)
		errx(EXIT_FAILURE, "Unable to allocate output buffer");
	out_buf_pos = 0;

	for (i = 0; i < rules_cnt; ++i) {
		if (RULE_SUBJOBJ(rules[i]) != RULE_SUBJOBJ(rule)) {
			if (RULE_SUBJOBJ(rule) && access)
				print_rule(labels[RULE_SUBJ(rule)]->label, labels[RULE_OBJ(rule)]->label, modestr[access]);
			RULE_SUBJOBJ(rule) = RULE_SUBJOBJ(rules[i]);
			access = 0;
		}

		if (rules[i].access2 >= 0) {
			// modify rule
			access |= rules[i].access1;
			access &= ~(rules[i].access2);
		} else {
			// set rule
			access = rules[i].access1;
		}
	}

	if (RULE_SUBJOBJ(rule) && access)
		print_rule(labels[RULE_SUBJ(rule)]->label, labels[RULE_OBJ(rule)]->label, modestr[access]);


	for (i = 0; i < out_buf_pos; i += ret) {
		ret = write(fd, out_buf + i, out_buf_pos - i);
		if (ret <= 0)
			err(EXIT_FAILURE, "Error while writing rules to kernel");
	}

	close(fd);
	free(out_buf);
}

/*
 * Function for writing rules to a legacy kernel, with no multi line support.
 * It duplicates some logic from print_rules(), but it makes both functions
 * quicker.
 */
void print_rules_legacy(int fd)
{
	struct rule rule;
	int access = 0;
	int i;
	int ret;
	FILE *f;

	RULE_SUBJOBJ(rule) = 0;
	f = fdopen(fd, "w");
	if (f == NULL)
		err(EXIT_FAILURE, "Unable to open %s for writing", SMACK_LOAD2_PATH);
	setvbuf(f, NULL, _IONBF, 0); /* Disable I/O buffering */

	for (i = 0; i < rules_cnt; ++i) {
		if (RULE_SUBJOBJ(rules[i]) != RULE_SUBJOBJ(rule)) {
			if (RULE_SUBJOBJ(rule) && access) {
				ret = fprintf(f, "%s %s %s", labels[RULE_SUBJ(rule)]->label, labels[RULE_OBJ(rule)]->label, modestr[access]);
				if (ret <= 0)
					err(EXIT_FAILURE, "Error while writing rules to kernel");
			}
			RULE_SUBJOBJ(rule) = RULE_SUBJOBJ(rules[i]);
			access = 0;
		}

		if (rules[i].access2 >= 0) {
			// modify rule
			access |= rules[i].access1;
			access &= ~(rules[i].access2);
		} else {
			// set rule
			access = rules[i].access1;
		}
	}

	if (RULE_SUBJOBJ(rule) && access) {
		ret = fprintf(f, "%s %s %s", labels[RULE_SUBJ(rule)]->label, labels[RULE_OBJ(rule)]->label, modestr[access]);
		if (ret <= 0)
			err(EXIT_FAILURE, "Error while writing rules to kernel");
	}

	fclose(f);
}

static int test_multiline(int *fd)
{
	*fd = open(SMACK_LOAD2_PATH, O_WRONLY);
	if (*fd < 0)
		err(EXIT_FAILURE, "Unable to open %s for writing", SMACK_LOAD2_PATH);

	if (write(*fd, "", 0) == 0)
		return 1; /* Multiline capable kernels can also handle 0-byte write */
	else
		return 0; /* Legacy kernels return an error */
}

void output(void)
{
	int fd;

	qsort(rules, rules_cnt, sizeof(struct rule), compare_rules);

	if (test_multiline(&fd))
		print_rules(fd);
	else
		print_rules_legacy(fd);
}
