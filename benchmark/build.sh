#!/bin/bash
BENCHMARKS="darknet msieve pc subset-sum"
for b in $BENCHMARKS; do
	pushd $b
	./build.sh
	popd
done
