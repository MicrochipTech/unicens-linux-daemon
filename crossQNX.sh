#!/bin/bash

# vim: ts=4 sw=4 et ft=sh

P="$(pwd)"
G="$P/generated"

# source the qnx cross environment
source ~/qnx700/qnxsdp-env.sh

# cleanup generated stuff
rm -rf "$G"

# generate a build environment in the generated/build/
mkdir -p "$G/build"
pushd "$G/build"
cmake -G "Unix Makefiles" \
    -DNO_RAW_CLOCK=on \
    -DQNX_BASE=$QNX_BASE \
    -DCMAKE_INSTALL_PREFIX="$G/dist" \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_TOOLCHAIN_FILE="$P/cmake-toolchain/qnx.cmake" \
    "$P"
popd

# make the binaries, libs, etc.
make -C "$G/build"
