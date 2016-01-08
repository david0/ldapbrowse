#pragma once

typedef struct TREENODE_S {
	char *value;
	struct TREENODE_S **children;
} TREENODE;

TREENODE *tree_node_alloc();

void tree_node_free(TREENODE * n);

unsigned tree_node_children_count(TREENODE * root);

void tree_node_append_child(TREENODE * root, TREENODE * child);
void tree_node_remove_childs(TREENODE * node);
