#!/bin/bash

for f in *S
do
  out="binaries/"`basename $f`
  gcc -c -m32 -o ${out}.o $f
  objcopy --output-target binary ${out}.o ${out}.raw
done
