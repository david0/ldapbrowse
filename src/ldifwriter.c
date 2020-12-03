#include "ldifwriter.h"
#include "ldapbrowse.h"

#include <stdio.h>
#include <stdlib.h>

void ldif_write(LDAP * ld, const char *filename, const char *dn, char **attributes)
{
	FILE *out = fopen(filename, "w");

	fputs("version: 1\n\n", out);

	LDAPMessage *msg;
	int errno = ldap_search_s(ld, dn, LDAP_SCOPE_SUB, "(objectClass=*)", attributes, 0, &msg);
	if (errno != LDAP_SUCCESS)
	{
		ldap_show_error(ld, errno, "ldap_search_s");
		return;
	}

	do
	{

		fputs("dn: ", out);
		char *entry_dn = ldap_get_dn(ld, msg);
		fputs(entry_dn, out);
		free(entry_dn);
		fputs("\n", out);

		BerElement *pber;
		char *attr;
		for (attr = ldap_first_attribute(ld, msg, &pber); attr != NULL;
		     ldap_memfree(attr), attr = ldap_next_attribute(ld, msg, pber))
		{
			char **values;
			if (!(values = ldap_get_values(ld, msg, attr)))
			{
				ldap_perror(ld, "ldap_get_values");
			}

			for (unsigned i = 0; values[i]; i++)
			{
				fputs(attr, out);
				fputs(": ", out);
				fputs(values[i], out);
				fputs("\n", out);
			}
			ldap_value_free(values);
		}
		ber_free(pber, 0);

		fputs("\n", out);
	}
	while ((msg = ldap_next_entry(ld, msg)) != NULL);

	ldap_msgfree(msg);

	fclose(out);
	out = NULL;
}
