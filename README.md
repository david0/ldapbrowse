# ldapbrowse 
simple LDAP browser with text user interface

[![Build Status](https://travis-ci.org/david0/ldapbrowse.png)](https://travis-ci.org/david0/ldapbrowse)

# Usage

The CLI is a subset of [ldapsearch](http://linux.die.net/man/1/ldapsearch):

      ldapbrowse  [-D binddn] [-W] [-w passwd] [-h ldaphost]  [-p ldapport]  [-b searchbase]  

# Compiling 

    cd src
    make

## dependencies

- ncurses
- libldap
- getopt


# Wishlist

- editing capabilities (add, delete, edit)
