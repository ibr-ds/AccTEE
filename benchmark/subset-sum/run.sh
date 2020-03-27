#!/bin/bash
if [ "$#" == 0 ]; then
	    n=1
elif [ "$#" == 1 ]; then
	    n=$1
else
	echo "wrong number of args"
	exit 1
fi
INPUT1="42 35"
INPUTFILE=../workloads/subset.txt
echo -n "subset-sum-native,"
for i in `seq $n`; do
	./subset-sum-clang $INPUT1 | grep It\ took\ me | cut -d" " -f4 | tr '\n' ','
done
export NODE_ENV=production
echo -en "\nsubset-sum-wasm-non,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.0.subset.wasm $INPUTFILE subset | grep It\ took\ me | cut -d" " -f4 | tr '\n' ','
done
echo -en "\nsubset-sum-wasm-naive,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.1.subset.wasm $INPUTFILE subset | grep It\ took\ me | cut -d" " -f4 | tr '\n' ','
done
echo -en "\nsubset-sum-wasm-flow-based,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.2.subset.wasm $INPUTFILE subset | grep It\ took\ me | cut -d" " -f4 | tr '\n' ','
done
echo -en "\nsubset-sum-wasm-loop-based,"
for i in `seq $n`; do
	node ../../middleware/init2.js instrumented.3.subset.wasm $INPUTFILE subset | grep It\ took\ me | cut -d" " -f4 | tr '\n' ','
done
