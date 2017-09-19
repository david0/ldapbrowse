CFLAGS=-g -Wall -std=c99 -D_BSD_SOURCE -DLDAP_DEPRECATED=1
LDFLAGS=-lncurses -lldap -lmenu -lform -llber -lm
OBJECTS=ldapbrowse.o tree.o treeview.o ldifwriter.o stringutils.o

ldapbrowse: $(OBJECTS)
	$(CC) $(CFLAGS) $* -o $@ $^ $(LDFLAGS)

clean:
	rm -f ldapbrowse $(OBJECTS)
