/**
	Copyright (c) 2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	main.cpp
	This file contains main() function, and helper function which checks if
	CPU supports required instruction set(s).

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

int appMain(int argc, char* argv[]);

#if defined(__i386__) || defined (__x86_64__)

#include <cpuid.h>
#include <stdio.h>

__attribute__((target("no-avx,no-sse")))
bool checkRequiredInstructionSets() {
#ifndef __SSE2__
	// Do not check CPU features if SSE2 is not required. Very old CPUs may not support CPUID.
	return true;
#else
	unsigned int a, b, c, d;

	if (!__get_cpuid(1, &a, &b, &c, &d)) {
		fprintf(stderr, "CPUID instruction is not supported by your CPU!\n");
		return false;
	}

#ifdef __SSE2__
	//printf("Checking for SSE2 support\n");
	if (0 == (d & bit_SSE2)) {
		fprintf(stderr, "SSE2 instructions are not supported by your CPU!\n");
		return false;
	}
#endif

#ifdef __AVX__
	//printf("Checking for AVX support\n");
	if (0 == (c & bit_AVX)) {
		fprintf(stderr, "AVX instructions are not supported by your CPU!\n");
		return false;
	}

	// AVX also needs OS support, check for it
	if (0 == (c & bit_OSXSAVE)) {
		fprintf(stderr, "OSXSAVE instructions are not supported by your CPU!\n");
		return false;
	}

	unsigned int eax, edx;
	unsigned int ecx = 0; // _XCR_XFEATURE_ENABLED_MASK
	__asm__ ("xgetbv" : "=a" (eax), "=d" (edx) : "c" (ecx));
	if (0x6 != (eax & 0x6)) { // XSTATE_SSE | XSTATE_YMM
		fprintf(stderr, "AVX instructions are not supported by your OS!\n");
		return false;
	}
#endif

#ifdef __FMA__
	//printf("Checking for FMA support\n");
	if (0 == (c & bit_FMA)) {
		fprintf(stderr, "FMA instructions are not supported by your CPU!\n");
		return false;
	}
#endif

#ifdef __AVX2__
	//printf("Checking for AVX2 support\n");
	if (__get_cpuid_max(0, 0) < 7) {
		fprintf(stderr, "Extended CPUID 0x7 instruction is not supported by your CPU!\n");
		return false;
	}

	__cpuid_count(7, 0, a, b, c, d);

	if (0 == (b & bit_AVX2)) {
		fprintf(stderr, "AVX2 instructions are not supported by your CPU!\n");
		return false;
	}
#endif

	return true;
#endif // !__SSE2__
}

__attribute__((target("no-avx,no-sse")))
int main(int argc, char* argv[]) {
    if (!checkRequiredInstructionSets())
        return 1;

    return appMain(argc, argv);
}

#else

#ifndef WASM
int main(int argc, char* argv[]) {
    return appMain(argc, argv);
}
#endif

#endif
