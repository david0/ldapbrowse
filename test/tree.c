#include <stdlib.h>
#include <assert.h>
#include "tree.h"

void test_add()
{
	TREENODE *root = tree_node_alloc(), *child = tree_node_alloc();

	tree_node_append_child(root, child);

	assert(tree_node_children_count(root) == 1);

	tree_node_free(root);
}

void test_remove_childs()
{
	TREENODE *root = tree_node_alloc();
	tree_node_remove_childs(root);
	assert(root != NULL);
	tree_node_free(root);
}

int main()
{
	test_add();
	test_remove_childs();
  return 0;
}
