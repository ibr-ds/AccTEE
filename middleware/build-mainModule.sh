#!/bin/bash
EMCC_FORCE_STDLIBS=dlmalloc,wasm-libc,libcxx,libcextra,libcxxabi,libc,libcxx_noexcept em++ mainModule.cpp --std=c++11 -O3 -s WASM=1 -s TOTAL_MEMORY=256MB -s MAIN_MODULE=1 -o mainModule.js

src=`cat mainModule.js`
echo "function InitMainModule() { 

	"$src"
	return Module;
}
exports.InitMainModule = InitMainModule;" > mainModule.js
