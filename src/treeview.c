#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <menu.h>
#include "treeview.h"

TREEVIEW *treeview_init(unsigned height, unsigned width)
{
	TREEVIEW *tv = calloc(1, sizeof(TREEVIEW));
	tv->width = width;
	tv->height = height;
	tv->toprow = 0;
	tv->win = newwin(height, width, 0, 0);
	return tv;
}

void treeview_free(TREEVIEW * tv)
{
	free(tv);
	tv = NULL;
}

void treeview_set_format(TREEVIEW * tv, unsigned rows, unsigned cols)
{
	tv->height = rows;
	tv->width = cols;
	wresize(tv->win, tv->height, tv->width);
}

void treeview_set_tree(TREEVIEW * tv, TREENODE * root)
{
	tv->root = root;
	tv->currentItemIndex = 0;
}

bool treeview_node_index(struct TREENODE_S *root, struct TREENODE_S *searchedNode, unsigned *index)
{
	if (!root)
		return false;

	if (searchedNode == root)
	{
		return true;
	}

	(*index)++;

	for (unsigned i = 0; root->children && root->children[i] != NULL; i++)
	{
		if (treeview_node_index(root->children[i], searchedNode, index))
		{
			return true;
		}
	}

	return NULL;
}

TREENODE *treeview_node_with_index_inner(struct TREENODE_S * node, unsigned requestedIndex,
					 unsigned *currentIndex)
{
	if (!node)
		return NULL;

	if (requestedIndex == *currentIndex)
	{
		return node;
	}

	(*currentIndex)++;

	for (unsigned i = 0; node->children && node->children[i] != NULL; i++)
	{
		TREENODE *result =
		    treeview_node_with_index_inner(node->children[i], requestedIndex, currentIndex);
		if (result)
		{
			return result;
		}
	}

	return NULL;
}

TREENODE *treeview_node_with_index(struct TREENODE_S * node, unsigned requestedIndex)
{
	unsigned index = 0;
	return treeview_node_with_index_inner(node, requestedIndex, &index);
}

TREENODE *treeview_current_node(TREEVIEW * tv)
{
	return treeview_node_with_index(tv->root, tv->currentItemIndex);
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
}

void treeview_set_current(TREEVIEW * tv, TREENODE * node)
{
	unsigned index = 0;
	bool result = treeview_node_index(tv->root, node, &index);
	if (result)
	{
		tv->currentItemIndex = index;
	}
	treeview_driver(tv, 0);
}

unsigned treeview_draw(TREEVIEW * tv, struct TREENODE_S *node, unsigned indent, unsigned index)
{
	unsigned oldIndex = index;
	if (!node)
		return 0;

	if (index >= tv->toprow)
	{
		wattrset(tv->win, 0);
		if (index == tv->currentItemIndex)
		{
			wattrset(tv->win, A_REVERSE);
		}

		for (unsigned i = 0; i < indent; i++)
		{
			waddstr(tv->win, " ");
		}
		waddstr(tv->win, node->value);

		for (unsigned i = 0; i < (tv->width - strlen(node->value) - indent); i++)
		{
			waddstr(tv->win, " ");
		}
	}
	index++;

	for (unsigned i = 0; node->children && node->children[i] != NULL; i++)
	{
		index += treeview_draw(tv, node->children[i], indent + 2, index);
	}
	return index - oldIndex;
}

unsigned treeview_num_nodes(TREEVIEW * tv)
{
	unsigned index = 0;
	treeview_node_with_index_inner(tv->root, -1, &index);
	return index;
}

void treeview_driver(TREEVIEW * tv, int c)
{
	switch (c)
	{
	case REQ_SCR_DPAGE:
		tv->currentItemIndex += tv->height;
		break;

	case REQ_SCR_UPAGE:
		tv->currentItemIndex -= tv->height;
		break;

	case REQ_DOWN_ITEM:
		tv->currentItemIndex++;
		break;

	case REQ_UP_ITEM:
		tv->currentItemIndex--;
		break;
	}

	tv->currentItemIndex = min(treeview_num_nodes(tv) - 1, tv->currentItemIndex);
	tv->currentItemIndex = max(0, tv->currentItemIndex);

	if (tv->currentItemIndex < tv->toprow)
	{
		tv->toprow = tv->currentItemIndex;
	}

	if (tv->currentItemIndex >= tv->toprow + tv->height)
	{
		tv->toprow = tv->currentItemIndex - tv->height + 1;
	}

	werase(tv->win);
	treeview_draw(tv, tv->root, 0, 0);
	wrefresh(tv->win);
}
