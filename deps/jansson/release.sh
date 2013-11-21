#!/bin/sh
#
# Use this script to easily make releases of Jansson. It configures
# the source tree, and builds and signs all tarballs.

die() {
    echo $1 >&2
    exit 1
}

confirm() {
    local answer
    read -p "$1 [yN]: " answer
    [ "$answer" = "Y" -o "$answer" = "y" ] || exit 0
}

set -e
[ -f configure.ac ] || die "Must be run at project root directory"

# Determine version
v=$(grep AC_INIT configure.ac | sed -r 's/.*, \[(.+?)\],.*/\1/')
[ -n "$v" ] || die "Unable to determine version"
confirm "Version is $v, proceed?"

# Sanity checks
vi=$(grep version-info src/Makefile.am | sed 's/^[ \t]*//g' | cut -d" " -f2)
confirm "Libtool version-info is $vi, proceed?"

r=$(grep 'Released ' CHANGES | head -n 1)
confirm "Last CHANGES entry says \"$r\", proceed??"

dv=$(grep ^version doc/conf.py | sed -r "s/.*'(.*)'.*/\1/")
if [ "$dv" != "$v" ]; then
    die "Documentation version ($dv) doesn't match library version"
fi

[ -f Makefile ] && make distclean || true
rm -f jansson-$v.tar.*
rm -rf jansson-$v-doc
rm -f jansson-$v-doc.tar.*

autoreconf -fi
./configure

# Run tests and make gz source tarball
: ${VALGRIND:=1}
export VALGRIND
make distcheck

# Make bzip2 source tarball
make dist-bzip2

# Sign source tarballs
for s in gz bz2; do
    gpg --detach-sign --armor jansson-$v.tar.$s
done

# Build documentation
make html
mv doc/_build/html jansson-$v-doc

# Make and sign documentation tarballs
for s in gz bz2; do
    [ $s = gz ] && compress=gzip
    [ $s = bz2 ] && compress=bzip2
    tar cf - jansson-$v-doc | $compress -9 -c > jansson-$v-doc.tar.$s
    gpg --detach-sign --armor jansson-$v-doc.tar.$s
done

echo "All done"
