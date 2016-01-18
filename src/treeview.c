#include <stdlib.h>
#include <assert.h>
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

		items[n - 2] = new_item(new, "");
		assert(items[n - 2] != NULL);
		set_item_userptr(items[n - 2], node);
		items[n - 1] = NULL;
	}

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
	TREEVIEW *tv = calloc(1, sizeof(TREEVIEW));
	tv->menu = new_menu(NULL);
	set_menu_mark(tv->menu, NULL);
	return tv;
}

void treeview_free_items(TREEVIEW * tv)
{
	unpost_menu(tv->menu);
	ITEM **items = menu_items(tv->menu);
	set_menu_items(tv->menu, NULL);
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
	free_menu(tv->menu);
	free(tv);
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
	unpost_menu(tv->menu);
	set_menu_format(tv->menu, rows, 1);
	tv->width = cols;
	treeview_post(tv);
}

void treeview_post(TREEVIEW * tv)
{
	tv->menu->namelen = tv->width;
	tv->menu->width = tv->width;
	tv->menu->itemlen = tv->width;
	post_menu(tv->menu);
}

void treeview_set_tree(TREEVIEW * tv, TREENODE * root)
{
	treeview_free_items(tv);
	int rc = set_menu_items(tv->menu, items_from_tree(root));
	assert(rc == E_OK);
	treeview_post(tv);
}

TREENODE *treeview_current_node(TREEVIEW * tv)
{
	return item_userptr(current_item(tv->menu));
}

int max(int a, int b)
{
	return a > b ? a : b; 
}

int min(int a, int b)
{
	return a < b ? a : b; 
}

void set_top_row_safe(TREEVIEW * tv, unsigned row)
{
	int max_toprow = max(item_count(tv->menu) - tv->menu->arows, 0);
	int rc = set_top_row(tv->menu, min(row, max_toprow));
	assert(rc == E_OK);
}

void treeview_set_current(TREEVIEW * tv, TREENODE * node)
{
	int rc = set_current_item(tv->menu, item_for_node(menu_items(tv->menu), node));
	assert(rc == E_OK);

	set_top_row_safe(tv, top_row(tv->menu));
}

void treeview_driver(TREEVIEW * tv, int c)
{
	menu_driver(tv->menu, c);
}
