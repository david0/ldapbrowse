#pragma once

#include <menu.h>
#include "tree.h"

typedef MENU TREEVIEW;

void treeview_render(TREENODE * root, void (expand_callback) (TREENODE *));
TREENODE *treeview_get_current();
