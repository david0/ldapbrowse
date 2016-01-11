CFLAGS=-I../src -g
LDFLAGS=-lmenu -lncurses

all: tests

tests: treeTest treeviewTest
.PHONY: tests

treeview: ../src/tree.o ../src/treeview.o

tree: ../src/tree.o

%Test: %
				@printf  "Runnig %-50s" $<...
				@valgrind --leak-check=full -q --error-exitcode=255 ./$<
				@echo "OK"

.PHONY: %Test
