#/bin/bash

if test -z "$srcdir"; then
    srcdir=echo "$0" | sed s,[^/]*$,,
    test "$srcdir" = "$0" && srcdir=.
    test -z "$srcdir" && srcdir=.
    test "${VERBOSE+set}" != set && VERBOSE=1
fi

./diablo_support_test_string_array > tmp_out
diff tmp_out $srcdir/data/diablo_support_test_string_array.out > /dev/null 2>/dev/null
RET=$?
rm tmp_out
exit $RET
