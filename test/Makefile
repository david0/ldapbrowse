CFLAGS=-I../src -g
LDFLAGS=-lmenu -lncurses
RUNNER:=valgrind -q --leak-check=full --error-exitcode=255

all: tests

tests: treeTest treeviewTest stringUtilsTest
.PHONY: tests

../src/%.o : ../src/%.c
				$(MAKE) -C ../src 
.PHONY: ../src/%.o

treeview: ../src/tree.o ../src/treeview.o treeview.o
				$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

tree: ../src/tree.o tree.o
				$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

stringUtilsTest: ../src/stringutils.o stringutils.o
				$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%Test: %
				@printf  "Runnig %-50s" $<...
				@$(RUNNER) ./$<
				@echo "OK"

.PHONY: %Test
