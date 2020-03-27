# SSE2, 64-bit
#ARCH += -march=core2 -mtune=core2 -m64
# AVX, 64-bit
#ARCH += -march=core2 -mtune=generic -msse4.2 -mpopcnt -maes -mpclmul -mavx -m64
# AVX+FMA, 64-bit
#ARCH += -march=core2 -mtune=generic -msse4.2 -mpopcnt -maes -mpclmul -mavx -mfma -m64
# AVX2+FMA, 64-bit
#ARCH += -march=core2 -mtune=generic -msse4.2 -mpopcnt -maes -mpclmul -mavx -mfma -mavx2 -m64

# 32-bit, no SIMD
#ARCH += -m32 -mno-sse
# SSE2, 32-bit
#ARCH += -march=core2 -mtune=core2 -m32
# AVX, 32-bit
#ARCH += -march=core2 -mtune=generic -msse4.2 -mpopcnt -maes -mpclmul -mavx -m32
# AVX+FMA, 32-bit
#ARCH += -march=core2 -mtune=generic -msse4.2 -mpopcnt -maes -mpclmul -mavx -mfma -m32

#CC = gcc
#CXX = g++

BOINC_DIR = ../../..
BOINC_API_DIR = $(BOINC_DIR)/api
BOINC_LIB_DIR = $(BOINC_DIR)/lib
BOINC_ZIP_DIR = $(BOINC_DIR)/zip
BOINC_LIBS = $(BOINC_API_DIR)/libboinc_api.a $(BOINC_LIB_DIR)/libboinc.a

BOINC_STUB=1
ifdef BOINC_STUB
BOINC_DIR = ../boinc_stub
BOINC_LIBS =
endif

#FOR WASM

#CFLAGS = -c -O3 $(ARCH) -Wall -Wextra -pedantic $(VARIANTFLAGS) -I$(BOINC_DIR) -I$(BOINC_LIB_DIR) -I$(BOINC_API_DIR) -I$(BOINC_ZIP_DIR) -I$(FREETYPE_DIR) -I./bz2 -Isimd -MMD -MP -DWASM
#LIBS = -static-libgcc -static-libstdc++ -pthread -Wl,-Bstatic ./bz2/libbz2.bc

#END WASM
#FOR NATIVE

CFLAGS = -c -O3 $(ARCH) -Wall -Wextra -pedantic -Werror $(VARIANTFLAGS) -I$(BOINC_DIR) -I$(BOINC_LIB_DIR) -I$(BOINC_API_DIR) -I$(BOINC_ZIP_DIR) -I$(FREETYPE_DIR) -Isimd -MMD -MP
LIBS = -static-libgcc -static-libstdc++ -pthread -Wl,-Bstatic -lbz2

#END NATIVE

FREETYPE_DIR = /usr/include/freetype2
CXXFLAGS = $(CFLAGS) -std=gnu++11
LDFLAGS = $(ARCH) -L/usr/X11R6/lib -L.
CXXSOURCES = BoincFile.cpp Graph.cpp boinc_functions.cpp utility.cpp pc.cpp main.cpp
CSOURCES = erf.c
OBJECTS = $(CXXSOURCES:.cpp=.o) $(CSOURCES:.c=.o)
EXECUTABLE = ../bin/pc

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CXX) $(LDFLAGS) $(OBJECTS) -o $@ $(LIBS) $(BOINC_LIBS)

.cpp.o:
	$(CXX) $(CXXFLAGS) $< -o $@

.c.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -rf ../bin/$(EXECUTABLE) *.o *~ *.d

.PHONY: all clean

-include $(CXXSOURCES:.cpp=.d) $(CSOURCES:.c=.d)
