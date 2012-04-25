#!/bin/sh

set -e
git log --stat --no-color > ChangeLog
intltoolize --automake --copy
libtoolize -q
aclocal -I m4
autoheader
automake --add-missing
autoconf
