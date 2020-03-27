#!/bin/bash
source ../instrument.sh

make clean
NO_ZLIB=1 CC=clang make all
mv msieve msieve.clang

make clean
NO_ZLIB=1 ADDITIONAL_CFLAGS="-DWASM" WASM_LIB_FLAGS=-L./gmp emmake make all
mv msieve msieve.bc
emcc msieve.bc -O3 -s TOTAL_MEMORY=256MB -s WASM=1 -s SIDE_MODULE=1  -s "EXPORTED_FUNCTIONS=['_allocateWorkload', '_launch']" -o msieve.wasm

instrument msieve.wasm
