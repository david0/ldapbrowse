#!/bin/sh


if [ ! -e unboundid-ldapsdk*/tools/in-memory-directory-server ]; then
  curl "https://docs.ldap.com/ldap-sdk/files/unboundid-ldapsdk-3.1.0-se.zip" -o unboundid.zip
  unzip -o unboundid.zip
fi

cd unboundid-ldap*
curl "https://raw.githubusercontent.com/ruby-ldap/ruby-net-ldap/master/test/testdata.ldif" -o testdata.ldif
echo connect with ldapbrowse -p 1234 -b "dc=bayshorenetworks,dc=com"
./tools/in-memory-directory-server --baseDN "dc=bayshorenetworks,dc=com" \
     --port 1234 \
     --ldifFile testdata.ldif 
