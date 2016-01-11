#include <stdlib.h>
#include <string.h>
#include "treeview.h"

ITEM **tree_item_dfs(TREENODE * node, ITEM ** items, int level)
{
	unsigned n;
	for (n = 0; items[n]; n++)
		;
	n++;

	items = realloc(items, ++n * sizeof(ITEM *));
	char *new = NULL;

	if (node->value)
	{
		unsigned indent = 2;
		new = malloc(level * indent + strlen(node->value) + 1);
		unsigned i;
		for (i = 0; i < level * indent; i++)
			new[i] = ' ';
		new[i] = '\0';

		strcat(new, node->value);
	}

	items[n - 2] = new_item(new, "");
	set_item_userptr(items[n - 2], node);
	items[n - 1] = NULL;

	for (unsigned i = 0; node->children && node->children[i]; i++)
		items = tree_item_dfs(node->children[i], items, level + 1);

	return items;
}

ITEM **items_from_tree(TREENODE * root)
{
	ITEM **i = calloc(1, sizeof(ITEM *));
	i = tree_item_dfs(root, i, 0);

	if (*i == NULL)
	{			// this would be eaten anyway by menu_items
		free(i);
		i = NULL;
	}

	return i;
}

TREEVIEW *treeview_init()
{
	return new_menu(NULL);
}

void treeview_free_items(TREEVIEW * tv)
{
	unpost_menu(tv);
	ITEM **items = menu_items(tv);
	set_menu_items(tv, NULL);
	for (unsigned i = 0; items && items[i]; i++)
	{
		free(item_name(items[i]));
		if (free_item(items[i]))
			fprintf(stderr, "free item failed\n");
	}
	free(items);
}

void treeview_free(TREEVIEW * tv)
{
	treeview_free_items(tv);
	free_menu(tv);
}

ITEM *item_for_node(ITEM ** item, TREENODE * n)
{
	do
	{
		if (item_userptr(*item) == n)
			return *item;
	}
	while (item++);

	return NULL;
}

void treeview_set_format(TREEVIEW * tv, unsigned rows, unsigned cols)
{
	set_menu_format(tv, rows, cols);
}

void treeview_post(TREEVIEW * tv)
{
	post_menu(tv);
}

void treeview_set_tree(TREEVIEW * tv, TREENODE * root)
{
	treeview_free_items(tv);
	set_menu_items(tv, items_from_tree(root));
	post_menu(tv);

}

TREENODE *treeview_current_node(TREEVIEW * tv)
{
	return item_userptr(current_item(tv));
}

void treeview_set_current(TREEVIEW * tv, TREENODE * node)
{
	set_current_item(tv, item_for_node(menu_items(tv), node));
}

void treeview_driver(TREEVIEW * tv, int c)
{
	menu_driver(tv, c);
}
