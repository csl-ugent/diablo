#!/bin/bash

echo diablo_object_test:
echo checks read and write of objects
echo checks symbol and relocation handling
echo checks link emulation

# Disabled for now
exit 0

if test -z "$srcdir"; then
    srcdir=`echo "$0" | sed s,[^/]*$,,`
    test "$srcdir" = "$0" && srcdir=.
    test -z "$srcdir" && srcdir=.
    test "${VERBOSE+set}" != set && VERBOSE=1
fi

./diablo_object_test -P $srcdir/../data -O $srcdir/inputs | grep DIFF > stdout
RET=$?

if [ $RET -ne 0 ]
then
	echo Test program did not return 0 as expected
	exit 1
fi

diff $srcdir/data/stdout stdout > /dev/null 2>/dev/null
RET=$?

if [ $RET -ne 0 ]
then
	echo Symbol/Relocation output differed
	exit 1
fi

if [ ! -e b.out ]
then
	echo b.out does not exist
	exit 1
fi

if [ ! -e c.out ]
then
	echo c.out does not exist
	exit 1
fi

chmod 700 b.out
./b.out > b.stdout
RET=$?
if [ $RET -ne 0 ] 
then
	echo Return value of b.out not correct
	exit $RET
fi


diff $srcdir/data/b.stdout b.stdout > /dev/null 2>/dev/null
RET=$?

if [ $RET -ne 0 ]
then
	echo Output of b.out differed
	exit 1
fi


cat build-config.h | grep '^#define BUILD_CPU *i[3456]86$' > /dev/null

if [ $RET -ne 0 ]
then
	echo Not executing, as cpu is not x86
	rm b.out c.out stdout
	exit 0
fi

chmod 700 c.out
./c.out > c.stdout
RET=$?
if [ $RET -ne 0 ] 
then
	echo Return value of c.out not correct
	exit $RET
fi

diff $srcdir/data/b.stdout c.stdout > /dev/null 2>/dev/null
RET=$?

if [ $RET -ne 0 ]
then
	echo Output of c.out differed
	exit 1
fi


rm b.out c.out stdout b.stdout c.stdout
exit 0
