/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	VectorTypeTraitsBase.hpp
	This file provides struct with few helper functions, useful when implementing
	traits struct for given SIMD instruction set.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _VECTOR_TYPE_TRAITS_BASE_HPP_
#define _VECTOR_TYPE_TRAITS_BASE_HPP_

template<typename VectorTypeTraits>
struct VectorTypeTraitsBase
{
	/*typedef T ElementType;
	typedef T VectorType;
	static const size_t ElementSize = sizeof(T);
	static const size_t VectorSize = sizeof(T);
	static const size_t ElementCount = 1;
	static const size_t DataAlignment = alignof(T);*/
	
	static size_t align_to_element_count(size_t value)
	{
		size_t remainder = value % VectorTypeTraits::ElementCount;
		if (0 != remainder)
		{
			value += VectorTypeTraits::ElementCount - remainder;
		}
		return value;
	}
	
	static size_t align_to_vector_size(size_t value)
	{
		size_t remainder = value % VectorTypeTraits::VectorSize;
		if (0 != remainder)
		{
			value += VectorTypeTraits::VectorSize - remainder;
		}
		return value;
	}
};

#endif // _VECTOR_TYPE_TRAITS_BASE_HPP_
