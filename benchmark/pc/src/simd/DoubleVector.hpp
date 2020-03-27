/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	DoubleVector.hpp
	This file checks which SIMD instructions are available, and defines various
	types and defines which will allows to best utilize available instruction
	sets.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _DOUBLE_VECTOR_HPP_
#define _DOUBLE_VECTOR_HPP_

#if defined(__i386__) || defined (__x86_64__)

#ifdef __AVX__
#include "AvxDoubleVectorTraits.hpp"
#endif
#ifdef __SSE2__
#include "SseDoubleVectorTraits.hpp"
#endif

#elif defined(__aarch64__)

#include "NeonDoubleVectorTraits.hpp"

#endif

#include "ScalarTypeTraits.hpp"
#include "Vector.hpp"

typedef Vector<ScalarTypeTraits<double>> ScalarDoubleVector;
typedef ScalarDoubleVector DoubleVector1;

#if defined(__i386__) || defined (__x86_64__)

#ifdef __AVX__
typedef Vector<AvxDoubleVectorTraits> AvxDoubleVector;
typedef AvxDoubleVector DoubleVector4;
#define HAS_DOUBLE_VECTOR_4 1
#endif

#ifdef __SSE2__
typedef Vector<SseDoubleVectorTraits> SseDoubleVector;
typedef SseDoubleVector DoubleVector2;
#define HAS_DOUBLE_VECTOR_2 1
#endif

#ifdef __AVX__
#ifdef __FMA__
#ifdef __AVX2__
#pragma message "Using FMA+AVX2 instructions"
#else
#pragma message "Using FMA+AVX instructions"
#endif
#else
#pragma message "Using AVX instructions"
#endif

typedef AvxDoubleVector DoubleVectorLong;
typedef SseDoubleVector DoubleVector;
#elif defined (__SSE2__)
#ifdef __FMA__
#pragma message "Using FMA+SSE2 instructions"
#else
#pragma message "Using SSE2 instructions"
#endif
typedef SseDoubleVector DoubleVector;
typedef DoubleVector DoubleVectorLong;
#else
typedef ScalarDoubleVector DoubleVector;
typedef DoubleVector DoubleVectorLong;
#endif

#elif defined(__aarch64__)

#pragma message "Using NEON instructions"

typedef Vector<NeonDoubleVectorTraits> NeonDoubleVector;
typedef NeonDoubleVector DoubleVector2;
typedef NeonDoubleVector DoubleVector;
typedef DoubleVector DoubleVectorLong;
#define HAS_DOUBLE_VECTOR_2 1

#else
typedef ScalarDoubleVector DoubleVector;
typedef DoubleVector DoubleVectorLong;
#endif

#endif // _DOUBLE_VECTOR_HPP_
