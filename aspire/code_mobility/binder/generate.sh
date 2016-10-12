#!/bin/bash

if [ "$#" -ne 3 ]; then
    echo "Illegal number of parameters"
	exit -1
fi

OPTIONS="-marm -DLINKIN_AFTER -std=gnu99 -Wall -Werror"

exec 1>Makefile

echo "CC = $1"
echo "OPTIONS = -O3 -fPIC $OPTIONS"
echo $'\r'
echo "default:"
echo $'\t''$(CC) $(OPTIONS)'" -o $2 -c binder.c"
echo $'\r'
echo $'\t''$(CC) $(OPTIONS)'" -o $3 -c downloader.c"
echo "clean:"
echo $'\t'"-rm -f $2 $3"
