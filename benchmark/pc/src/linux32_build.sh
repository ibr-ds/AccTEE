#!/bin/bash
# This script assume that the PC++ is inside the 'boinc-repo'
# (boinc/samples/pc-boinc/src)


echo "**********************"
echo "* BUILD BOINC 32 bit *"
echo "**********************"

cd ../../../
sh ./_autosetup
sh ./configure --disable-client --disable-server LDFLAGS=-static-libgcc CXXFLAGS=-m32
make

echo "*****************************"
echo "* BUILD PC++ (BOINC) 32 bit *"
echo "*****************************"

cd samples/pc-boinc/src/
ln -sf `g++ -m32 -print-file-name=libstdc++.a`
make clean
export ARCH=-m32
make
