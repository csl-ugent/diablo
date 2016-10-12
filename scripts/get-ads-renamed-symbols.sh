#!/bin/bash

# pass the output of armlink -v (version 3.1 and up, stdout) through this
# script and pipe the output to <binary>.symtrans. Diablo (for arm) will
# then read this file and perform the same symbol renamings as the linker.
egrep "^Renaming symbol " | sed -e 's/Renaming symbol \([^ ]*\) to \([^ ]*\)\./\1 \2/'|tr ' ' '\n' 
