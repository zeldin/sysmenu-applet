#!/bin/sh

set -e
git log --stat --no-color -C -C -M > ChangeLog
intltoolize --automake --copy
libtoolize -q --copy
aclocal -I m4
autoheader
automake --add-missing --copy
autoconf
