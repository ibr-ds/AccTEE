all: native clang wasm

native:
	g++ -std=c++11 subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -DTIMESTAMP -O3 -o subset-sum-gcc

clang:
	clang++ -std=c++11 subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -DTIMESTAMP -O3 -o subset-sum-clang

wasm:
	emcc -std=c++11 -DWASM subset_sum_main.cpp common/binary_output.cpp common/generate_subsets.cpp common/n_choose_k.cpp -O3 -s SIDE_MODULE=1 -s WASM=1 -o subset.wasm
