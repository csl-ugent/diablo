#!/bin/bash
set -o errexit
set -o pipefail
set -o nounset
#set -o xtrace

# Get the repo and build directories
repo_dir=$(dirname $0)
obj_dir=$1
mkdir -p $obj_dir

# Build the print objects
cd $repo_dir/self-profiling
./generate.sh /opt/diablo-gcc-toolchain/bin/arm-diablo-linux-gnueabi-cc Makefile.arm_linux arm
make -f Makefile.arm_linux
mv print.o $obj_dir/printarm_linux.o
./generate.sh /opt/diablo-android-gcc-toolchain/bin/arm-linux-androideabi-gcc Makefile.arm_android arm
make -f Makefile.arm_android
cp print.o $obj_dir/printarm_android.o
