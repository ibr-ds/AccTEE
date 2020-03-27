#!/bin/bash
source ../instrument.sh
emmake make

mv darknet darknet.bc
emcc darknet.bc -O3 -s TOTAL_MEMORY=256MB -s WASM=1 -s SIDE_MODULE=1 -o darknet.wasm

instrument darknet.wasm
