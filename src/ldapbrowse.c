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
WINDOW *attrwin;
char **attributes;

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

void ldap_show_error(LDAP * ld, int errno, const char *s)
{
	char *errstr = ldap_err2string(errno);
	werase(attrwin);
	waddstr(attrwin, s);
	waddstr(attrwin, ": ");
	waddstr(attrwin, errstr);
	wrefresh(attrwin);
}

void ldap_load_subtree(TREENODE * root)
{
	LDAPMessage *msg;
	char *dn = node_dn(root);
	int errno = ldap_search_s(ld, dn, LDAP_SCOPE_ONE, "(objectClass=*)", NULL, 0, &msg);
	if (errno != LDAP_SUCCESS)
	{
		ldap_show_error(ld, errno, "ldap_search_s");
		return;
	}

	free(dn);
	dn = NULL;

	tree_node_remove_childs(root);

	LDAPMessage *entry;
	for (entry = ldap_first_entry(ld, msg); entry != NULL; entry = ldap_next_entry(ld, entry))
	{
		TREENODE *child = tree_node_alloc();
		char *dn = ldap_get_dn(ld, entry);
		char **dns = ldap_explode_dn(dn, 0);
		child->value = strdup(dns[0]);
		tree_node_append_child(root, child);
		ldap_value_free(dns);
	}

	ldap_msgfree(msg);
}

void selection_changed(WINDOW * win, TREENODE * selection)
{
	werase(win);
	LDAPMessage *msg;
	char *dn = node_dn(selection);
	int errno = ldap_search_s(ld, dn, LDAP_SCOPE_BASE, "(objectClass=*)", attributes, 0, &msg);
	if (errno != LDAP_SUCCESS)
	{
		ldap_show_error(ld, errno, "ldap_search_s");
		return;
	}

	waddstr(win, "dn: ");
	waddstr(win, dn);
	waddstr(win, "\n");

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
		ldap_value_free(values);
	}

	ber_free(pber, 0);
	ldap_msgfree(msg);

	wrefresh(win);
}

TREENODE *ldap_delete_subtree(TREENODE * root, TREENODE * selected_node)
{
	LDAPMessage *msg;
	char *dn = node_dn(selected_node);
	int errno = ldap_delete_s(ld, dn);
	free(dn);
	dn = NULL;

	if (errno != LDAP_SUCCESS)
	{
		ldap_show_error(ld, errno, "ldap_delete_s");
		return NULL;
	}

	TREENODE *parent = tree_node_get_parent(root, selected_node);
	if (parent)
	{
		// reload
		tree_node_remove_childs(parent);
		ldap_load_subtree(parent);
		treeview_set_tree(treeview, root);
		treeview_set_current(treeview, parent);

		selection_changed(attrwin, parent);
	}

	return parent;
}

void render(TREENODE * root, void (expand_callback) (TREENODE *))
{
	int height, width;
	getmaxyx(stdscr, height, width);
	attrwin = newwin(height / 2 - 1, width, height / 2 + 1, 0);
	treeview = treeview_init();

	treeview_set_tree(treeview, root);
	treeview_set_format(treeview, height / 2, width);
	treeview_post(treeview);

	refresh();

	mvhline(height / 2, 0, 0, width);

	selection_changed(attrwin, treeview_current_node(treeview));

	int c;
	while ((c = getch()) != 'q')
	{
		TREENODE *selected_node = treeview_current_node(treeview);

		switch (c)
		{
		case KEY_RESIZE:
			getmaxyx(stdscr, height, width);

			treeview_set_format(treeview, height / 2, width);
			treeview_set_current(treeview, selected_node);

			wresize(attrwin, height / 2 - 1, width);
			mvwin(attrwin, height / 2 + 1, 0);

			break;
		case KEY_UP:
			treeview_driver(treeview, REQ_UP_ITEM);
			selection_changed(attrwin, treeview_current_node(treeview));
			break;

		case KEY_DOWN:
			treeview_driver(treeview, REQ_DOWN_ITEM);
			selection_changed(attrwin, treeview_current_node(treeview));
			break;

		case KEY_PPAGE:
			treeview_driver(treeview, REQ_SCR_UPAGE);
			selection_changed(attrwin, treeview_current_node(treeview));
			break;

		case KEY_NPAGE:
			treeview_driver(treeview, REQ_SCR_DPAGE);
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
				if (tree_node_children_count(selected_node) == 0)
					selected_node = tree_node_get_parent(root, selected_node);

				if (selected_node)
				{
					tree_node_remove_childs(selected_node);

					treeview_set_tree(treeview, root);
					treeview_set_current(treeview, selected_node);
				}
			}
			break;

		case 'D':
			{
				werase(attrwin);
				waddstr(attrwin, "do you really want to delete?");
				wrefresh(attrwin);
				if (getch() == 'y')
					selected_node = ldap_delete_subtree(root, selected_node);

			}
			break;

		}

		mvhline(height / 2, 0, 0, width);

	}

	treeview_free(treeview);
	treeview = NULL;

}

int main(int argc, char *argv[])
{
	char *ldap_host = "127.0.0.1";
	char *bind_dn = NULL;
	struct berval passwd = { 0, NULL };
	char *base = NULL;
	unsigned port = 389;
	char *ldap_uri = NULL;

	while (true)
	{
		char c = getopt(argc, argv, "H:h:p:w:D:b:");

		if (c == -1)	// check for end of options
			break;

		switch (c)
		{
		case 'H':
			ldap_uri = optarg;
			break;

		case 'h':
			ldap_host = optarg;
			break;

		case 'p':
			port = atoi(optarg);
			break;

		case 'w':
			passwd.bv_val = optarg;
			passwd.bv_len = strlen(optarg);
			break;

		case 'D':
			bind_dn = optarg;
			break;

		case 'b':
			base = optarg;
			break;

		default:
			exit(-1);
		}
	}

	if ((argc - optind) > 0)
	{
		attributes = argv + optind;
	}

	if (ldap_uri == NULL)
	{
		asprintf(&ldap_uri, "ldap://%s:%d", ldap_host, port);
	}

	if (ldap_initialize(&ld, ldap_uri) != LDAP_SUCCESS)
	{
		ldap_perror(ld, "ldap_initialize failed");
		exit(EXIT_FAILURE);
	}

	int version = LDAP_VERSION3;
	if (ldap_set_option(ld, LDAP_OPT_PROTOCOL_VERSION, &version) != LDAP_OPT_SUCCESS)
	{
		ldap_perror(ld, "ldap_set_option");
		exit(EXIT_FAILURE);
	}

	int msgid = 0;
	if (ldap_sasl_bind(ld, bind_dn, LDAP_SASL_SIMPLE, &passwd, NULL, NULL, &msgid) !=
	    LDAP_SUCCESS)
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
			ldap_perror(ld, "ldap_get_values");
			exit(EXIT_FAILURE);
		}

		if (!(values = ldap_get_values(ld, msg, "namingContexts")))
		{
			ldap_perror(ld, "ldap_get_values");
		}

		if (values)
			base = values[0];

		ldap_msgfree(msg);
		msg = NULL;
	}

	if (!base)
	{
		printf("no baseDn given and server does not support namingContexts\n");
		exit(EXIT_FAILURE);
	}

	TREENODE *root = tree_node_alloc();
	root->value = strdup(base);

	ldap_load_subtree(root);

	curses_init();

	render(root, ldap_load_subtree);

	endwin();
}
