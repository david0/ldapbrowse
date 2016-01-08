CFLAGS=-g -Wall -DLDAP_DEPRECATED=1
LDFLAGS=-lcurses -lldap -lmenu -llber
OBJECTS=ldapbrowse.o tree.o treeview.o

ldapbrowse: $(OBJECTS)
	$(CC) $(CFLAGS) $(LDFLAGS) $* -o $@ $^

clean:
	rm -f ldapbrowse $(OBJECTS)
