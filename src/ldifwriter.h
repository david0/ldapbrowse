#ifndef _LDIFWRITER_H_
#define _LDIFWRITER_H_
#include <ldap.h>

void ldif_write(LDAP * ld, const char *filename, const char *dn, char **attributes);

#endif
