#!/bin/sh

aclocal -I aclocal
automake --add --copy --add-missing
autoheader
autoconf
automake --add-missing
