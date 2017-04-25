#include <stdlib.h>
#include <string.h>
#include "stringutils.h"

char *string_after_last(char *str, char c)
{
	char *last = 0;
	for (char *cur = str; *cur != 0; cur++)
	{
		if (*cur == c)
			last = cur;
	}

	return ++last;
}

char *string_before(char *str, char c)
{
	int len;
	for (len = 0; str[len] != NULL && str[len] != c; len++)
		;

	char *result = (char *)malloc(sizeof(char) * (len + 1));
	strncpy(result, str, len);
	result[len] = 0;
	return result;
}

char *trim_whitespaces(char *str)
{
	// trim leading space
	while (isspace(*str))
		str++;

	if (*str == 0)		// all spaces?
		return str;

	// trim trailing space
	char *end = str + strnlen(str, 128) - 1;

	while (end > str && isspace(*end))
		end--;

	// write new null terminator
	*(end + 1) = '\0';

	return str;
}
