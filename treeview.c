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
	return tree_item_dfs(root, i, 0);
}

MENU *treeview_menu_init(TREENODE * root)
{
	return new_menu((ITEM **) items_from_tree(root));
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

void update_menu_items(MENU * menu, TREENODE * root)
{
	unpost_menu(menu);
	ITEM **items = menu_items(menu);
	set_menu_items(menu, NULL);
	free(items);

	set_menu_items(menu, items_from_tree(root));
	post_menu(menu);

}

void treeview_render(TREENODE * root, void (expand_callback) (TREENODE *))
{
	MENU *menu = treeview_menu_init(root);

	set_menu_format(menu, LINES / 2, 1);

	post_menu(menu);
	refresh();

	int c;
	while ((c = getch()) != KEY_F(1))
	  {
		  TREENODE *selected_node = item_userptr(current_item(menu));

		  switch (c)
		    {
		    case KEY_DOWN:
			    menu_driver(menu, REQ_DOWN_ITEM);
			    break;

		    case KEY_RIGHT:
			    {
				    expand_callback(selected_node);
				    /*
				       TREENODE *new_node = tree_node_alloc();
				       asprintf(&(new_node->value), "%s sub", selected_node->value);
				       tree_node_append_child(selected_node, new_node);                             
				     */
				    update_menu_items(menu, root);
				    set_current_item(menu,
						     item_for_node(menu_items(menu),
								   selected_node));
				    break;
			    }

		    case KEY_LEFT:
			    {
				    tree_node_remove_childs(selected_node);

				    update_menu_items(menu, root);
				    set_current_item(menu,
						     item_for_node(menu_items(menu),
								   selected_node));
			    }

			    break;
		    case KEY_UP:
			    menu_driver(menu, REQ_UP_ITEM);
			    break;
		    }
	  }

	free(menu);
}
