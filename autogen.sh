#!/bin/sh

set -e
intltoolize --automake --copy
libtoolize -q
aclocal -I m4
autoheader
automake --foreign --add-missing
autoconf
