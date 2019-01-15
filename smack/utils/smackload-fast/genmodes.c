#include <stdio.h>
#include "common.h"

int main(void)
{
	int mode;

	printf("const char *modestr[] = {");
	for (mode = 0; mode < MODE_MAX; ++mode) {
		char str[32];
		int i = 0;

		if (mode & MODE_READ)
			str[i++] = 'r';
		if (mode & MODE_WRITE)
			str[i++] = 'w';
		if (mode & MODE_EXEC)
			str[i++] = 'x';
		if (mode & MODE_APPEND)
			str[i++] = 'a';
		if (mode & MODE_TRANS)
			str[i++] = 't';
		if (mode & MODE_LOCK)
			str[i++] = 'l';
		str[i] = '\0';

		if (mode == 0)
			printf("\"-\"");
		else
			printf(", \"%s\"", (i > 0) ? str : "-");
	}
	printf("};\n");

	return 0;
}
