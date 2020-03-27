#!/bin/bash
export NODE_ENV=production
if [  $# -ne 2 ]; then 
	echo "usage: $0 mainModule-file wasm-file"
	exit 1
fi 
#backup mainModule
cp ../../middleware/mainModule.wasm ../../middleware/mainModule.wasm.backup
cp -f $1 ../../middleware/mainModule.wasm 2>/dev/null

node ../../middleware/init2.js $2 ../workloads/dog.txt darknet

#restore mainModule
mv ../../middleware/mainModule.wasm.backup ../../middleware/mainModule.wasm
