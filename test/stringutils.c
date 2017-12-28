#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "stringutils.h"

void test_string_after_last()
{
	char *dn = "dc=root";
	assert(strcmp("root", string_after_last(dn, '=')) == 0);
}

void test_string_before()
{
	char *dn = "o=bar,dc=root";
	char *before = string_before(dn, ',');
	assert(strcmp("o=bar", before) == 0);
	free(before);
}

int main()
{
	test_string_after_last();
	test_string_before();
	return 0;
}
