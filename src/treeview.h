#pragma once

#include <curses.h>
#include "tree.h"

typedef struct TREEVIEW {
	TREENODE *root;
	WINDOW *win;
	unsigned currentItemIndex;
	unsigned width;
	unsigned height;
	unsigned toprow;
} TREEVIEW;

TREEVIEW *treeview_init();

void treeview_free(TREEVIEW * root);

void treeview_set_format(TREEVIEW * tv, unsigned rows, unsigned cols);

/**
 * Rebuilds the whole tree with root as new tree root.
 * Selection is lost during this.
 **/
void treeview_set_tree(TREEVIEW * tv, TREENODE * root);

/**
 * Return the currently selected tree node
 **/
TREENODE *treeview_current_node(TREEVIEW * tv);

void treeview_set_current(TREEVIEW * tv, TREENODE * node);

void treeview_driver(TREEVIEW * tv, int c);
