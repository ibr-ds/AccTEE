#!/bin/bash
# This script assume that the PC++ is inside the 'boinc-repo'
# (boinc/samples/pc-boinc/src)


echo "**********************"
echo "* BUILD BOINC 64 bit *"
echo "**********************"

cd ../../../
sh ./_autosetup
sh ./configure --disable-client --disable-server LDFLAGS=-static-libgcc
make

echo "*****************************"
echo "* BUILD PC++ (BOINC) 64 bit *"
echo "*****************************"

cd samples/pc-boinc/src/
ln -sf `g++ -print-file-name=libstdc++.a`
make clean
make
