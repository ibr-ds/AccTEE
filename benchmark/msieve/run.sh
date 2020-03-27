#!/bin/bash
if [ "$#" == 0 ]; then
	    n=1
elif [ "$#" == 1 ]; then
	    n=$1
else
	echo "wrong number of args"
	exit 1
fi
FILE=../workloads/msieve.txt
INPUT=`cat $FILE`
echo -n "msieve-native,"
for i in `seq $n`; do
	rm -f msieve.dat
	./msieve.clang $INPUT | grep Duration | cut -d" " -f 2 | tr '\n' ','
done
export NODE_ENV=production
echo -en "\nmsieve-wasm-non,"
for i in `seq $n`; do
	rm -f msieve.dat
	node ../../middleware/init2.js instrumented.0.msieve.wasm $FILE msieve | grep Duration | cut -d" " -f 2 | tr '\n' ','
done
echo -en "\nmsieve-wasm-naive,"
for i in `seq $n`; do
	rm -f msieve.dat
	node ../../middleware/init2.js instrumented.1.msieve.wasm $FILE msieve | grep Duration | cut -d" " -f 2 | tr '\n' ','
done
echo -en "\nmsieve-wasm-flow-based,"
for i in `seq $n`; do
	rm -f msieve.dat
	node ../../middleware/init2.js instrumented.2.msieve.wasm $FILE msieve | grep Duration | cut -d" " -f 2 | tr '\n' ','
done
echo -en "\nmsieve-wasm-loop-based,"
for i in `seq $n`; do
	rm -f msieve.dat
	node ../../middleware/init2.js instrumented.3.msieve.wasm $FILE msieve | grep Duration | cut -d" " -f 2 | tr '\n' ','
done
