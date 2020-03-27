#!/bin/bash
source ../instrument.sh
mkdir bin

pushd src
make clean
make -f Makefile
popd
mv bin/pc pc-native

pushd src
make clean
emmake make -f Makefile-wasm
popd
mv bin/pc pc.bc
emcc pc.bc -O3 -s TOTAL_MEMORY=256MB -s WASM=1 -s SIDE_MODULE=1 -s "EXPORTED_FUNCTIONS=['_allocateWorkload', '_launch']" -o pc.wasm
mv bin/pc.wasm .
instrument pc.wasm
