#include <ldap.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <getopt.h>

#include <curses.h>
#include <form.h>
#include <menu.h>
#include "tree.h"
#include "ldifwriter.h"
#include "treeview.h"
#include "stringutils.h"

#define KEY_ENTER_MAC 0x0a
#define KEY_ESC 0x1b

LDAP *ld;
TREEVIEW *treeview;
WINDOW *attrpad;
int attrpad_toprow = 0, attrpad_rows = 0;
char **attributes;

struct INPUT_DIALOG {
	WINDOW *win;
	FORM *form;
	FIELD *fields[2];
};

void resize();

void curses_init()
{
	setenv("ESCDELAY", "0", 0);
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
	werase(attrpad);
	waddstr(attrpad, s);
	waddstr(attrpad, ": ");
	waddstr(attrpad, errstr);
	wrefresh(attrpad);
}

void ldap_load_subtree_filtered(TREENODE * root, const char *filter)
{
	LDAPMessage *msg;
	char *dn = node_dn(root);
	int errno = ldap_search_s(ld, dn, LDAP_SCOPE_ONE, filter, NULL, 0, &msg);
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
		free(dn);
		dn = NULL;
	}

	ldap_msgfree(msg);
}

void ldap_load_subtree(TREENODE * root)
{
	ldap_load_subtree_filtered(root, "(objectClass=*)");
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

	attrpad_rows = 1;
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
			continue;
		}

		for (unsigned i = 0; values[i]; i++)
		{
			waddstr(win, attr);
			waddstr(win, ": ");
			waddstr(win, values[i]);
			waddstr(win, "\n");
			attrpad_rows++;
		}
		ldap_value_free(values);
	}

	ber_free(pber, 0);
	ldap_msgfree(msg);

	int height, width;
	getmaxyx(stdscr, height, width);
	int attrpad_height = height / 2 + 1;

	// limit scrolling
	if (attrpad_toprow < 0)
		attrpad_toprow = 0;
	if (attrpad_toprow > attrpad_rows + 2 - attrpad_height)
		attrpad_toprow = attrpad_rows + 2 - attrpad_height;

	prefresh(win, attrpad_toprow, 0, attrpad_height, 0, height - 1, width - 1);
}

struct INPUT_DIALOG input_dialog_create(const char *description, const char *placeholder)
{
	struct INPUT_DIALOG dlg;
	int height, width;
	getmaxyx(stdscr, height, width);
	int winheight = 10, winwidth = width - 10;
	dlg.win = newwin(winheight, winwidth, (height - winheight) / 2, 2);
	box(dlg.win, 0, 0);

	dlg.fields[0] = new_field(1, winwidth - 5, 3, 2, 0, 0);
	dlg.fields[1] = NULL;

	set_field_buffer(dlg.fields[0], 0, placeholder);

	curs_set(1);

	mvwaddstr(dlg.win, 1, 2, description);
	dlg.form = new_form(dlg.fields);
	set_form_win(dlg.form, dlg.win);
	set_form_sub(dlg.form, derwin(dlg.win, winheight - 3, winwidth - 2, 2, 1));
	post_form(dlg.form);

	wrefresh(dlg.win);
	return dlg;
}

void input_dialog_close(struct INPUT_DIALOG dlg)
{
	curs_set(0);

	unpost_form(dlg.form);
	free_form(dlg.form);
	dlg.form = NULL;
	free_field(dlg.fields[0]);
	dlg.fields[0] = NULL;
}

char *input_dialog_value(struct INPUT_DIALOG dlg)
{
	return strdup(trim_whitespaces(field_buffer(dlg.fields[0], 0)));
}

char *input_dialog(const char *description, const char *placeholder)
{
	struct INPUT_DIALOG dlg = input_dialog_create(description, placeholder);

	int ch;
	while (ch = getch(), (ch != KEY_ENTER) && (ch != KEY_ENTER_MAC) && (ch != KEY_ESC))
	{
		switch (ch)
		{
		case KEY_RESIZE:
			{
				form_driver(dlg.form, REQ_VALIDATION);
				char *current_value = input_dialog_value(dlg);
				input_dialog_close(dlg);
				clear();
				resize();
				refresh();
				dlg = input_dialog_create(description, current_value);
				break;
			}
		case KEY_LEFT:
			form_driver(dlg.form, REQ_PREV_CHAR);
			break;

		case KEY_RIGHT:
			form_driver(dlg.form, REQ_NEXT_CHAR);
			break;
			// Delete the char before cursor
		case KEY_BACKSPACE:
		case 127:
			form_driver(dlg.form, REQ_DEL_PREV);
			break;

			// Delete the char under the cursor
		case KEY_DC:
		case 74:
			form_driver(dlg.form, REQ_DEL_CHAR);
			break;

		default:
			form_driver(dlg.form, ch);
			break;
		}

		wrefresh(dlg.win);
	}
	form_driver(dlg.form, REQ_VALIDATION);

	char *result = NULL;
	bool canceled = (ch == KEY_ESC);
	if (!canceled)
	{
		result = input_dialog_value(dlg);
	}

	input_dialog_close(dlg);

	return result;
}

void filtered_search(TREENODE * selected_node)
{
	char *filter = input_dialog("Filter:", "(objectClass=*)");
	if (filter)
	{
		ldap_load_subtree_filtered(selected_node, filter);
		free(filter);
		filter = NULL;
	}
}

void ldap_save_subtree(TREENODE * selected_node)
{

	char *nameSuggestion = NULL;
	char *dn = node_dn(selected_node);
	char *rdn = string_before(dn, ',');
	asprintf(&nameSuggestion, "%s.ldif", string_after_last(rdn, '='));
	free(dn);
	dn = NULL;
	free(rdn);
	rdn = NULL;

	char *filename = input_dialog("Save as:", nameSuggestion);
	if (filename)
	{
		char *dn = node_dn(selected_node);
		ldif_write(ld, filename, dn, attributes);
		free(dn);
		free(filename);
		filename = NULL;
	}

	free(nameSuggestion);
	nameSuggestion = NULL;

}

TREENODE *ldap_delete_subtree(TREENODE * root, TREENODE * selected_node)
{
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

		selection_changed(attrpad, parent);
	}

	return parent;
}

void resize()
{
	int height, width;
	getmaxyx(stdscr, height, width);
	treeview_set_format(treeview, height / 2, width);

	refresh();
	treeview_driver(treeview, 0);
	mvhline(height / 2, 0, 0, width);
	attrpad_toprow = 0;
	selection_changed(attrpad, treeview_current_node(treeview));
}

void render(TREENODE * root, void (expand_callback) (TREENODE *))
{
	int height, width;
	getmaxyx(stdscr, height, width);
	attrpad = newpad(1000, 1000);
	treeview = treeview_init(height / 2, width);

	treeview_set_tree(treeview, root);

	refresh();
	treeview_driver(treeview, 0);

	mvhline(height / 2, 0, 0, width);

	selection_changed(attrpad, treeview_current_node(treeview));

	int c;
	while ((c = getch()) != 'q')
	{
		TREENODE *selected_node = treeview_current_node(treeview);

		switch (c)
		{
		case KEY_RESIZE:
			resize();
			break;
		case KEY_UP:
			treeview_driver(treeview, REQ_UP_ITEM);
			attrpad_toprow = 0;
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case KEY_DOWN:
			treeview_driver(treeview, REQ_DOWN_ITEM);
			attrpad_toprow = 0;
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case 'o':
			attrpad_toprow++;
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case 'p':
			attrpad_toprow--;
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case KEY_PPAGE:
			treeview_driver(treeview, REQ_SCR_UPAGE);
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case KEY_NPAGE:
			treeview_driver(treeview, REQ_SCR_DPAGE);
			selection_changed(attrpad, treeview_current_node(treeview));
			break;

		case KEY_RIGHT:
			{
				expand_callback(selected_node);
				treeview_driver(treeview, 0);
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
				werase(attrpad);
				waddstr(attrpad, "do you really want to delete?");
				wrefresh(attrpad);
				if (getch() == 'y')
					selected_node = ldap_delete_subtree(root, selected_node);

			}
			break;

		case 's':
			{
				ldap_save_subtree(selected_node);

				treeview_driver(treeview, 0);
				selection_changed(attrpad, treeview_current_node(treeview));
			}
			break;

		case 'f':
			{
				filtered_search(treeview_current_node(treeview));
				treeview_driver(treeview, 0);
				selection_changed(attrpad, treeview_current_node(treeview));
			}
			break;
		}

		getmaxyx(stdscr, height, width);
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
	int deref = LDAP_DEREF_NEVER;

	while (true)
	{
		char c = getopt(argc, argv, "H:h:p:w:D:b:a:");

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
			base = strdup(optarg);
			break;

		case 'a':
			if (strcasecmp("never", optarg) == 0)
			{
				deref = LDAP_DEREF_NEVER;
			} else if (strcasecmp("always", optarg) == 0)
			{
				deref = LDAP_DEREF_ALWAYS;
			} else if (strcasecmp("find", optarg) == 0)
			{
				deref = LDAP_DEREF_FINDING;
			} else if (strcasecmp("search", optarg) == 0)
			{
				deref = LDAP_DEREF_SEARCHING;
			} else
			{
				fprintf(stderr, "%s is not a valid value for option a\n", optarg);
				exit(-1);
			}

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
	} else
	{
		ldap_uri = strdup(ldap_uri);
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

	if (ldap_set_option(ld, LDAP_OPT_DEREF, &deref) != LDAP_OPT_SUCCESS)
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
		{
			base = strdup(values[0]);
		}

		for (unsigned i = 0; values[i]; i++)
		{
			free(values[i]);
			values[i] = NULL;
		}
		free(values);
		values = NULL;

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

	tree_node_remove_childs(root);
	free(root->value);
	free(root);
	root = NULL;

	free(base);

	free(ldap_uri);
	ldap_uri = NULL;
}
