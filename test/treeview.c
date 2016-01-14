#include <assert.h>
#include <string.h>
#include "treeview.h"

void test_create_set_tree_free()
{
	TREEVIEW *tv = treeview_init();
	TREENODE *root = tree_node_alloc();

	treeview_set_tree(tv, root);

	treeview_free(tv);
	tree_node_free(root);
}

void test_create_add_free()
{
	TREENODE *root = tree_node_alloc(), *child = tree_node_alloc();

	root->value = strdup("root");
	child->value = strdup("child");

	tree_node_append_child(root, child);

	TREEVIEW *tv = treeview_init();
	treeview_set_tree(tv, root);

	treeview_free(tv);
	tree_node_free(root);
}

int main()
{
	initscr();		// needed because stdscr must be set with curses 5.9

	test_create_set_tree_free();
	test_create_add_free();

	endwin();
	return 0;
}
