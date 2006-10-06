#!/bin/sh

clear
reset -w

PROGRAM=dit

make=make
[ -e /bin/ColorMake ] && make=ColorMake

if [ -e $PROGRAM ] && [ ! -e mode.txt -o "`cat mode.txt`" != "$1" ]
then
   cp $PROGRAM $PROGRAM.old
   make clean
   mv $PROGRAM.old $PROGRAM
fi
mode=`echo "$@" | tr ' ' '\n' | grep -v -- --`
echo $mode > mode.txt

HEADERS=(Prototypes.h Structures.h)
for h in "${HEADERS[@]}"
do
   [ -e $h ] && mv $h $h.old
done
./GenHeaders
for h in "${HEADERS[@]}"
do
   diff --text $h $h.old &>/dev/null && mv $h.old $h
done
./MakeMakefile $PROGRAM "$@" > Makefile
$make $mode
