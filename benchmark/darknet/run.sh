#!/bin/bash
# 0 is not instrumented
# 1 is instrumented, opt level 0
# 2 is instrumented, opt level 1
# 3 is instrumented, opt level 2
echo -n "wasm0,"
for i in `seq 1 10`; do
	./darknet.sh ../../middleware/mainModule.wasm darknet.wasm | grep Entire\ duration | cut -d" " -f3 | tr '\n' ','
done
echo ""

echo -n "wasm1,"
for i in `seq 1 10`; do
	./darknet.sh ../../middleware/instrumented.0.mainModule.wasm instrumented.0.darknet.wasm | grep Entire\ duration | cut -d" " -f3 | tr '\n' ','
done
echo ""

echo -n "wasm2,"
for i in `seq 1 10`; do
	./darknet.sh ../../middleware/instrumented.1.mainModule.wasm instrumented.1.darknet.wasm | grep Entire\ duration | cut -d" " -f3 | tr '\n' ','
done
echo ""

echo -n "wasm3,"
for i in `seq 1 10`; do
	./darknet.sh ../../middleware/instrumented.2.mainModule.wasm instrumented.2.darknet.wasm | grep Entire\ duration | cut -d" " -f3 | tr '\n' ','
done
echo ""
