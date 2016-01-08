#include <ldap.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <curses.h>
#include <menu.h>
#include "tree.h"
#include "treeview.h"

LDAP *ld;

void curses_init()
{
	initscr();
	clear();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
}

void ldap_load_subtree(TREENODE * root)
{
	LDAPMessage *msg;
	if (ldap_search_s(ld, root->value, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg)
	    != LDAP_SUCCESS)
	  {
		  ldap_perror(ld, "ldap_search_s");
	  }

	tree_node_remove_childs(root);

	LDAPMessage *entry;
	for (entry = ldap_first_entry(ld, msg); entry != NULL; entry = ldap_next_entry(ld, entry))
	  {
		  TREENODE *child = tree_node_alloc();
		  child->value = ldap_get_dn(ld, entry);
		  tree_node_append_child(root, child);
	  }
}

int main(int argc, char *argv[])
{
	curses_init();

	char *ldap_host;
	char *bind_dn;
	char *root_pw;
	char *base;

	if (argc < 4)
	  {
		  fprintf(stderr, "USAGE: %s [host] [binddn] [passwd]\n", argv[0]);
		  return -1;
	  }

	ldap_host = argv[1];
	bind_dn = argv[2];
	root_pw = argv[3];

	if ((ld = ldap_init(ldap_host, 16611)) == NULL)
	  {
		  perror("ldap_init failed");
		  exit(EXIT_FAILURE);
	  }

	if (ldap_bind_s(ld, bind_dn, root_pw, LDAP_AUTH_SIMPLE) != LDAP_SUCCESS)
	  {
		  ldap_perror(ld, "ldap_bind");
		  exit(EXIT_FAILURE);
	  }

	LDAPMessage *msg;
	if (ldap_search_s(ld, "", LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg) != LDAP_SUCCESS)
	  {
		  ldap_perror(ld, "ldap_search_s");
	  }

	char **values;
	if (!(values = ldap_get_values(ld, msg, "namingContexts")))
	  {
		  ldap_perror(ld, "ldap_get_values");
	  }

	base = values[0];
	assert(base != NULL);

	if (ldap_search_s(ld, base, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg)
	    != LDAP_SUCCESS)
	  {
		  ldap_perror(ld, "ldap_search_s");
	  }

	TREENODE *root = tree_node_alloc();
	root->value = strdup(base);

	ldap_load_subtree(root);

	treeview_render(root, ldap_load_subtree);

	endwin();
}
