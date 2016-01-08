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
TREEVIEW *treeview;

void curses_init()
{
	initscr();
	clear();
	noecho();
	cbreak();
	keypad(stdscr, TRUE);
	curs_set(0);
}

void ldap_load_subtree(TREENODE * root)
{
	LDAPMessage *msg;
	if (ldap_search_s(ld, root->value, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg)
	    != LDAP_SUCCESS)
		ldap_perror(ld, "ldap_search_s");

	tree_node_remove_childs(root);

	LDAPMessage *entry;
	for (entry = ldap_first_entry(ld, msg); entry != NULL; entry = ldap_next_entry(ld, entry))
	  {
		  TREENODE *child = tree_node_alloc();
		  child->value = ldap_get_dn(ld, entry);
		  tree_node_append_child(root, child);
	  }
}

void hrule(unsigned line)
{
	move(line, 0);
	for (unsigned col = 0; col < COLS; col++)
		addch('-');
}

void selection_changed(WINDOW * win, TREENODE * selection)
{
	wmove(win, 0, 0);
	LDAPMessage *msg;
	if (ldap_search_s(ld, selection->value, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg)
	    != LDAP_SUCCESS)
		ldap_perror(ld, "ldap_search_s");

	LDAPMessage *entry;
	BerValue *pber;
	char *attr;
	for (attr = ldap_first_attribute(ld, msg, &pber); attr != NULL;
	     attr = ldap_next_attribute(ld, entry, pber))
	  {
		  waddstr(win, attr);
		  waddstr(win, "\n\r");
	  }
	ber_free(pber, 0);
	wrefresh(win);
}

void render(TREENODE * root, void (expand_callback) (TREENODE *))
{
	WINDOW *attrwin = newwin(LINES / 2 - 1, COLS, LINES / 2 + 1, 0);
	treeview = treeview_init(root);

	treeview_set_format(treeview, LINES / 2, 1);
	treeview_post(treeview);

	refresh();

	hrule(LINES / 2 + 1);

	int c;
	while ((c = getch()) != KEY_F(1))
	  {
		  TREENODE *selected_node = treeview_current_node(treeview);

		  switch (c)
		    {
		    case KEY_DOWN:
			    treeview_driver(treeview, REQ_DOWN_ITEM);
			    selection_changed(attrwin, selected_node);
			    break;

		    case KEY_RIGHT:
			    {
				    expand_callback(selected_node);

				    treeview_set_tree(treeview, root);
				    treeview_set_current(treeview, selected_node);
				    break;
			    }

		    case KEY_LEFT:
			    {
				    tree_node_remove_childs(selected_node);

				    treeview_set_tree(treeview, root);
				    treeview_set_current(treeview, selected_node);
			    }

			    break;
		    case KEY_UP:
			    treeview_driver(treeview, REQ_UP_ITEM);
			    selection_changed(attrwin, selected_node);
			    break;
		    }

		  hrule(LINES / 2 + 1);

	  }

	free(treeview);
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

	render(root, ldap_load_subtree);

	endwin();
}
