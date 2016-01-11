CFLAGS=-g -Wall -DLDAP_DEPRECATED=1 -std=c99
LDFLAGS=-lncurses -lldap -lmenu -llber
OBJECTS=ldapbrowse.o tree.o treeview.o

ldapbrowse: $(OBJECTS)
	$(CC) $(CFLAGS) $* -o $@ $^ $(LDFLAGS)

clean:
	rm -f ldapbrowse $(OBJECTS)
