#!/bin/bash
# Starts an in-memory-ldap browser with an predefined set of test dataa

SCRIPTDIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"
ZIPFILE="${SCRIPTDIR}/unboundid.zip"
TESTFILE="${SCRIPTDIR}/testdata.ldif"

cd "${SCRIPTDIR}"

if [ ! -e $SCRIPTDIR/unboundid-ldapsdk*/tools/in-memory-directory-server ]; then
  curl "https://docs.ldap.com/ldap-sdk/files/unboundid-ldapsdk-3.1.0-se.zip" -o "${ZIPFILE}"
  unzip -o ${ZIPFILE}
fi

[ ! -e "${TESTFILE}" ] && curl "https://raw.githubusercontent.com/ruby-ldap/ruby-net-ldap/master/test/testdata.ldif" -o "${TESTFILE}"

cd unboundid-ldapsdk*

echo connect with ldapbrowse -p 1234 -b "dc=bayshorenetworks,dc=com"
./tools/in-memory-directory-server --baseDN "dc=bayshorenetworks,dc=com" \
     --port 1234 \
     --ldifFile $TESTFILE
