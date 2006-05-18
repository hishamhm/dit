#!/bin/sh

clear
reset -w

PROGRAM=dit

make=make
[ -e /bin/ColorMake ] && make=ColorMake

if [ ! -e mode.txt -o "`cat mode.txt`" != "$1" ]
then
   cp $PROGRAM $PROGRAM.old
   make clean
   mv $PROGRAM.old $PROGRAM
fi
echo "$1" > mode.txt

HEADERS=(Prototypes.h Structures.h)
for h in "${HEADERS[@]}"
do
   mv $h $h.old
done
./GenHeaders
for h in "${HEADERS[@]}"
do
   diff --text $h $h.old &>/dev/null && mv $h.old $h
done
./MakeMakefile $PROGRAM > Makefile
$make $1
