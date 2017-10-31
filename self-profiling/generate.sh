#!/bin/bash

if [ "$#" -ne 3 ]; then
  echo "Illegal number of parameters. Expected a path to a Diablo-compatible compiler, a name for the print object, and an architecture."
  exit -1
fi

# Get the arguments
cc=$1
makefile=$2

# Decrypt the flags argument
flags=""
case $3 in
	"arm" | "thumb")
	flags="-marm"
	;;

	"i486")
	;;

	*)
	echo "Illegal architecture"
	exit -1
	;;
esac

sed -e "s#TEMPLATE_CC#$cc#g" -e "s/TEMPLATE_FLAGS/$flags/g" Makefile.template > $makefile
