#include <ldap.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>

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

char *node_dn(TREENODE * node)
{
	char *dn = calloc(1, 1);
	while (node)
	{
		unsigned len = strlen(dn) + strlen(node->value) + 2;
		dn = realloc(dn, len);

		strcat(dn, ",");
		strcat(dn, node->value);

		node = node->parent;
	}

	for (unsigned i = 0; dn[i]; i++)
		dn[i] = dn[i + 1];

	return dn;
}

void ldap_load_subtree(TREENODE * root)
{
	LDAPMessage *msg;
	char *dn = node_dn(root);
	if (ldap_search_s(ld, dn, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg) != LDAP_SUCCESS)
		ldap_perror(ld, "ldap_search_s");

	free(dn);
	dn = NULL;

	tree_node_remove_childs(root);

	LDAPMessage *entry;
	for (entry = ldap_first_entry(ld, msg); entry != NULL; entry = ldap_next_entry(ld, entry))
	{
		TREENODE *child = tree_node_alloc();
		char *dn = ldap_get_dn(ld, entry);
		char **dns = ldap_explode_dn(dn, 0);
		child->value = dns[0];
		tree_node_append_child(root, child);
	}
}

void selection_changed(WINDOW * win, TREENODE * selection)
{
	werase(win);
	LDAPMessage *msg;
	char *dn = node_dn(selection);
	if (ldap_search_s(ld, dn, LDAP_SCOPE_BASE, "(objectClass=*)", NULL, 0, &msg) != LDAP_SUCCESS)
		ldap_perror(ld, "ldap_search_s");

	free(dn);
	dn = NULL;

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
			waddstr(win, attr);
			waddstr(win, ": ");
			waddstr(win, values[i]);
			waddstr(win, "\n");
		}
	}

	ber_free(pber, 0);
	ldap_msgfree(msg);

	wrefresh(win);
}

void render(TREENODE * root, void (expand_callback) (TREENODE *))
{
	WINDOW *attrwin = newwin(LINES / 2 - 1, COLS, LINES / 2 + 1, 0);
	treeview = treeview_init();

	treeview_set_tree(treeview, root);
	treeview_set_format(treeview, LINES / 2, COLS);
	treeview_post(treeview);

	refresh();

	mvhline(LINES / 2, 0, 0, COLS);

	selection_changed(attrwin, root);

	int c;
	while ((c = getch()) != 'q')
	{
		TREENODE *selected_node = treeview_current_node(treeview);

		switch (c)
		{
		case KEY_UP:
			treeview_driver(treeview, REQ_UP_ITEM);
			selection_changed(attrwin, treeview_current_node(treeview));
			break;

		case KEY_DOWN:
			treeview_driver(treeview, REQ_DOWN_ITEM);
			selection_changed(attrwin, treeview_current_node(treeview));
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

		}

		mvhline(LINES / 2, 0, 0, COLS);

	}

	treeview_free(treeview);
	treeview = NULL;

}

int main(int argc, char *argv[])
{
	char *ldap_host = "127.0.0.1";
	char *bind_dn = "";
	char *password = "";
	char *base = NULL;
	unsigned port = 389;

	while (true)
	{
		int option_index = 0;
		char c = getopt(argc, argv, "h:p:w:D:b:");

		if (c == -1)	// check for end of options
			break;

		switch (c)
		{
		case 'h':
			ldap_host = optarg;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case 'w':
			password = optarg;
			break;

		case 'D':
			bind_dn = optarg;
			break;

		case 'b':
			base = optarg;
			break;

		default:
			abort();
		}
	}

	if ((ld = ldap_init(ldap_host, 16611)) == NULL)
	{
		perror("ldap_init failed");
		exit(EXIT_FAILURE);
	}

	if (ldap_bind_s(ld, bind_dn, password, LDAP_AUTH_SIMPLE) != LDAP_SUCCESS)
	{
		ldap_perror(ld, "ldap_bind");
		exit(EXIT_FAILURE);
	}

	LDAPMessage *msg;
	if (!base)
	{
		char **values;

		if (ldap_search_s(ld, "", LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg) !=
		    LDAP_SUCCESS)
		{
			ldap_perror(ld, "ldap_search_s");
			exit(EXIT_FAILURE);
		}

		if (!(values = ldap_get_values(ld, msg, "namingContexts")))
		{
			ldap_perror(ld, "ldap_get_values");
		}

		base = values[0];
	}

	if (!base)
		printf("no baseDn given and server does not supportt namingContexts\n");

	if (ldap_search_s(ld, base, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg)
	    != LDAP_SUCCESS)
	{
		ldap_perror(ld, "ldap_search_s");
	}

	TREENODE *root = tree_node_alloc();
	root->value = strdup(base);

	ldap_load_subtree(root);

	curses_init();

	render(root, ldap_load_subtree);

	endwin();
}
