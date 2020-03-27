#!/bin/bash
if [ "$#" == 0 ]; then
	    n=1
elif [ "$#" == 1 ]; then
	    n=$1
else
	echo "wrong number of args"
	exit 1
fi
ARGS_NATIVE="input/tile.txt output/output2.txt 0.05 1 2470 0"
FILE_WASM=../workloads/pc.txt

echo -n "pc-native,"
for i in `seq $n`; do
	bin/pc-native $ARGS_NATIVE | grep Finished | cut -d" " -f3 | tr '\n' ','
done
export NODE_ENV=production
echo -en "\npc-wasm-non,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.0.pc.wasm $FILE_WASM "pc" | grep Finished | cut -d" " -f3 | tr '\n' ','
done
echo -en "\npc-wasm-naive,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.1.pc.wasm $FILE_WASM "pc" | grep Finished | cut -d" " -f3 | tr '\n' ','
done
echo -en "\npc-wasm-flow-based,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.2.pc.wasm $FILE_WASM "pc" | grep Finished | cut -d" " -f3 | tr '\n' ','
done
echo -en "\npc-wasm-loop-based,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.3.pc.wasm $FILE_WASM "pc" | grep Finished | cut -d" " -f3 | tr '\n' ','
done
