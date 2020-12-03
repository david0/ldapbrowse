#pragma once
#include <ldap.h>

void ldap_show_error(LDAP * ld, int errno, const char *s);

