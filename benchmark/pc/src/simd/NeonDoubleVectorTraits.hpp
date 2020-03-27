/**
	Copyright (c) 2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	NeonDoubleVectorTraits.hpp
	File with helper struct with definitions of various NEON SIMD operations.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _NEON_DOUBLE_VECTOR_TRAITS_HPP_
#define _NEON_DOUBLE_VECTOR_TRAITS_HPP_

#include <arm_neon.h>
#include "VectorTypeTraitsBase.hpp"

template<typename T>
struct ScalarTypeTraits;

struct NeonDoubleVectorTraits : public VectorTypeTraitsBase<NeonDoubleVectorTraits>
{
	typedef double ElementType;
	typedef float64x2_t VectorType;
	
	typedef double HalfVectorType;
	typedef ScalarTypeTraits<ElementType> HalfVectorTraits;
	
	static const size_t ElementSize = sizeof(ElementType);
	static const size_t VectorSize = 16;
	//static const size_t ElementCount = VectorSize / ElementSize;
	static const size_t ElementCount = 2;
	static const size_t DataAlignment = 16;
	
	// load vector from memory
	static VectorType load(const ElementType* mem_addr)
	{
		return vld1q_f64(static_cast<ElementType*>(__builtin_assume_aligned(mem_addr, DataAlignment)));
	}
	
	// store vector in memory
	static void store(ElementType* mem_addr, const VectorType& a)
	{
		vst1q_f64(static_cast<ElementType*>(__builtin_assume_aligned(mem_addr, DataAlignment)), a);
	}
	
	// create a vector with all elements equal to 0.0
	static VectorType setzero()
	{
		return vdupq_n_f64(0.0);
	}
	
	// create a vector with all elements equal to A
	static constexpr VectorType set1(const ElementType A)
	{
		//return vdupq_n_f64(A);
		return VectorType{ A, A };
	}
	
	// create a vector with given elements
	static VectorType set(const ElementType A, const ElementType B)
	{
		return VectorType{ B, A };
	}
	
	// returns a+b
	static VectorType add(const VectorType& a, const VectorType& b)
	{
		return vaddq_f64(a, b);
	}
	
	// returns a-b
	static VectorType sub(const VectorType& a, const VectorType& b)
	{
		return vsubq_f64(a, b);
	}
	
	// returns a*b
	static VectorType mul(const VectorType& a, const VectorType& b)
	{
		return vmulq_f64(a, b);
	}
	
	// returns a/b
	static VectorType div(const VectorType& a, const VectorType& b)
	{
		return vdivq_f64(a, b);
	}
	
	// returns sqrt(a) (square root)
	static VectorType sqrt(const VectorType& a)
	{
		return vsqrtq_f64(a);
	}
	
	// returns a*b+c
	static VectorType mul_add(const VectorType& a, const VectorType& b, const VectorType& c)
	{
		//return vfmaq_f64(a, b, c);
		return add(mul(a, b), c);
	}
	
	// returns -(a*b)+c
	static VectorType neg_mul_add(const VectorType& a, const VectorType& b, const VectorType& c)
	{
		// -(a*b)+c == c - (a * b)
		//return vfmsq_f64(a, b, c);
		return sub(c, mul(a, b));
	}
	
	static HalfVectorType get_lower_half(const VectorType& a)
	{
		return a[0];
	}
	
	static HalfVectorType get_upper_half(const VectorType& a)
	{
		return a[1];
	}
	
	// returns sum of vector elements
	static ElementType sum_elements(const VectorType& a)
	{
		return a[0] + a[1];
	}
	
	static ElementType* alloc(size_t element_count)
	{
		element_count = align_to_element_count(element_count);
		return static_cast<ElementType*>(aligned_alloc(
			DataAlignment, ElementSize * element_count));
	}
	
	static void free(ElementType* ptr)
	{
		::free(ptr);
	}
	
	static ElementType get_at(const VectorType& v, size_t index)
	{
		return v[index];
	}
};

#endif // _NEON_DOUBLE_VECTOR_TRAITS_HPP_
