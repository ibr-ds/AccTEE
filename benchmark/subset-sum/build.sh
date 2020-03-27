#!/bin/bash
source ../instrument.sh
g++ -std=c++11 subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -DTIMESTAMP -O3 -o subset-sum-gcc

clang++ -std=c++11 subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -DTIMESTAMP -O3 -o subset-sum-clang
emcc -std=c++11 -DWASM subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -O3 -s SIDE_MODULE=1 -s WASM=1 -s "EXPORTED_FUNCTIONS=['_allocateWorkload', '_launch']" -o subset.wasm

instrument subset.wasm
