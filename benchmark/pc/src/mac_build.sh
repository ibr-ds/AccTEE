#!/bin/bash
# Build PC++ (BOINC) for Mac


export MAC_OS_X_VERSION_MAX_ALLOWED=1012
export MAC_OS_X_VERSION_MIN_REQUIRED=1090

cd ../../../mac_build/
source setupForBoinc.sh
source BuildMacBOINC.sh -lib
cd ../samples/pc-boinc/src/

GPPPATH=`xcrun -find g++`
GCCPATH=`xcrun -find gcc`

export CXX="${GPPPATH}"
export CC="${GCCPATH}"
export VARIANTFLAGS="-arch x86_64"
export MACOSX_DEPLOYMENT_TARGET=10.9

make -f Makefile_mac clean
make -f Makefile_mac
