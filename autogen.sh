#!/bin/sh

aclocal -I aclocal
automake --add --copy
autoheader
autoconf
automake --add-missing

