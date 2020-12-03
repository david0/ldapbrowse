#pragma once
#include <ldap.h>

void ldif_write(LDAP * ld, const char *filename, const char *dn, char **attributes);
