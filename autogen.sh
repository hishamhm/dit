#!/bin/sh

aclocal
automake --add --copy
autoheader
autoconf
automake
