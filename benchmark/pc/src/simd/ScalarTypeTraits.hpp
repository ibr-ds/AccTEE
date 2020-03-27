/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	ScalarTypeVectorTraits.hpp
	File with helper struct with definitions of various scalar double precisions
	operations. It is designed to work as a 1-element vector. This allows to
	create code which is independent of actual vector size.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _SCALAR_TYPE_TRAITS_HPP_
#define _SCALAR_TYPE_TRAITS_HPP_

#include <math.h>
#include "VectorTypeTraitsBase.hpp"

template<typename T>
struct ScalarTypeTraits : public VectorTypeTraitsBase<ScalarTypeTraits<T> >
{
	typedef T ElementType;
	typedef T VectorType;
	
	// this is not supported
	typedef void HalfVectorType;
	typedef void HalfVectorTraits;
	
	static const size_t ElementSize = sizeof(T);
	static const size_t VectorSize = sizeof(T);
	static const size_t ElementCount = 1;
	static const size_t DataAlignment = alignof(T);
	
	// load vector from memory
	static VectorType load(const ElementType* mem_addr)
	{
		return *mem_addr;
	}
	
	// store vector in memory
	static void store(ElementType* mem_addr, const VectorType& a)
	{
		*mem_addr = a;
	}
	
	// create a vector with all elements equal to 0.0
	static VectorType setzero()
	{
		return ElementType();
	}
	
	// create a vector with all elements equal to A
	static constexpr VectorType set1(const ElementType A)
	{
		return A;
	}
	
	// returns a+b
	static VectorType add(const VectorType& a, const VectorType& b)
	{
		return a + b;
	}
	
	// returns a-b
	static VectorType sub(const VectorType& a, const VectorType& b)
	{
		return a - b;
	}
	
	// returns a*b
	static VectorType mul(const VectorType& a, const VectorType& b)
	{
		return a * b;
	}
	
	// returns a/b
	static VectorType div(const VectorType& a, const VectorType& b)
	{
		return a / b;
	}
	
	// returns sqrt(a) (square root)
	static VectorType sqrt(const VectorType& a)
	{
		return ::sqrt(a);
	}
	
	// returns a*b+c
	static VectorType mul_add(const VectorType& a,
		const VectorType& b, const VectorType& c)
	{
		return add(mul(a, b), c);
	}
	
	// returns -(a*b)+c
	static VectorType neg_mul_add(const VectorType& a, const VectorType& b, const VectorType& c)
	{
		// -(a*b)+c == c - (a * b)
		return sub(c, mul(a, b));
	}
	
	// returns sum of vector elements
	static ElementType sum_elements(const VectorType& a)
	{
		return a;
	}
	
	static ElementType* alloc(size_t element_count)
	{
		return static_cast<ElementType*>(malloc(ElementSize * element_count));
	}
	
	static void free(ElementType* ptr)
	{
		::free(ptr);
	}
	
	static ElementType get_at(const VectorType& v, size_t /*index*/)
	{
		return v;
	}
};

#endif // _SCALAR_TYPE_TRAITS_HPP_
