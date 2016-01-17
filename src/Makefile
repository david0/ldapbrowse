CFLAGS=-g -Wall -std=c99 -D_BSD_SOURCE -DLDAP_DEPRECATED=1
LDFLAGS=-lncurses -lldap -lmenu -llber
OBJECTS=ldapbrowse.o tree.o treeview.o

ldapbrowse: $(OBJECTS)
	$(CC) $(CFLAGS) $* -o $@ $^ $(LDFLAGS)

clean:
	rm -f ldapbrowse $(OBJECTS)
