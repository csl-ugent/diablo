#/bin/bash

if test -z "$srcdir"; then
    srcdir=echo "$0" | sed s,[^/]*$,,
    test "$srcdir" = "$0" && srcdir=.
    test -z "$srcdir" && srcdir=.
    test "${VERBOSE+set}" != set && VERBOSE=1
fi

./diablo_support_test_init -h | grep -- "-h" >/dev/null
RET=$? 
if [ $RET -ne 0 ]
then
    exit $RET
fi
./diablo_support_test_init -h | grep -- "-V" >/dev/null
RET=$? 
if [ $RET -ne 0 ]
then
    exit $RET
fi
./diablo_support_test_init -V | grep -- version > /dev/null
RET=$? 
if [ $RET -ne 0 ]
then
    exit $RET
fi
exit 0 
