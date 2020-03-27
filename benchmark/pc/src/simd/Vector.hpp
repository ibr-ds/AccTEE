/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	Vector.hpp
	Generic implementation of SIMD Vector. It uses helper traits struct, which
	provides actual implementations for all operations.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _VECTOR_HPP_
#define _VECTOR_HPP_

#include <type_traits>
#include <cassert>

// std::negation requires C++17, so defined own one
template<typename B>
struct negation : public std::integral_constant<bool, !B::value>
{};

template<typename VectorTypeTraits>
class Vector
{
public:
	typedef typename VectorTypeTraits::ElementType ElementType;
	typedef typename VectorTypeTraits::VectorType VectorType;
	
	typedef typename VectorTypeTraits::HalfVectorTraits HalfVectorTypeTraits;
	
	static const size_t ElementSize = VectorTypeTraits::ElementSize;
	static const size_t VectorSize = VectorTypeTraits::VectorSize;
	static const size_t ElementCount = VectorTypeTraits::ElementCount;
	static const size_t DataAlignment = VectorTypeTraits::DataAlignment;
	
private:
	typedef Vector<VectorTypeTraits> ThisType;
	typedef Vector<HalfVectorTypeTraits> HalfVectorType;
	
	VectorType value;
	
public:
	
	Vector()
		: value(VectorTypeTraits::setzero())
	{}
	
	explicit constexpr Vector(const ElementType& val)
		: value(VectorTypeTraits::set1(val))
	{}
	
	explicit Vector(const ElementType* val_ptr)
		: value(VectorTypeTraits::load(val_ptr))
	{}
	
	// When scalar type pretends to be vector type, we would get two identical constructors.
	// Used SFINAE to to eliminate this one in such case.
	template<typename VT = VectorType, typename T2 = typename std::enable_if<negation<
		std::is_same<ElementType, VT>>::value>::type>
	explicit Vector(const VectorType& val)
		: value(val)
	{}
	
	Vector(const ThisType& other)
		: value(other.value)
	{}
	
	// Create vector instance from column of array
	Vector(ElementType** ptr, size_t row, size_t col)
	{
		loadColumn(ptr, row, col);
	}
	
	// Constructors which allows to create vectors by specyfying their elements one by one.
	// Another place where SFINAE is required.
	template<typename VTT = VectorTypeTraits, typename T2 = typename std::enable_if<2 == VTT::ElementCount>::type>
	explicit Vector(const ElementType& A, const ElementType& B)
		: value(VectorTypeTraits::set(A, B))
	{}
	
	template<typename VTT = VectorTypeTraits, typename T2 = typename std::enable_if<4 == VTT::ElementCount>::type>
	explicit Vector(const ElementType& A, const ElementType& B, const ElementType& C, const ElementType& D)
		: value(VectorTypeTraits::set(A, B, C, D))
	{}
	
	ThisType& operator=(const ThisType& other)
	{
		value = other.value;
		return *this;
	}
	
	void load(const ElementType* val_ptr)
	{
		value = VectorTypeTraits::load(val_ptr);
	}
	
	void store(ElementType* val_ptr) const
	{
		VectorTypeTraits::store(val_ptr, value);
	}
	
	void loadColumn(ElementType** ptr, size_t row, size_t col)
	{
		ElementType vec[ElementCount] __attribute__((aligned(DataAlignment)));
		
		// GCC had bug and did not align variables on stack using requested alignment.
		// It should be fixed starting from GCC 4.6, but check for it to be sure.
		// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=16660
		// Make sure to comment this out before compiling final version!
		assert(0 == (static_cast<uintptr_t>(&vec[0]) & DataAlignment));
		
		for (size_t n = 0; n < ElementCount; ++n)
			vec[n] = ptr[row + n][col];
		value = VectorTypeTraits::load(vec);
	}
	
	ThisType operator+(const ThisType& other) const
	{
		return ThisType(VectorTypeTraits::add(value, other.value));
	}
	
	ThisType operator-(const ThisType& other) const
	{
		return ThisType(VectorTypeTraits::sub(value, other.value));
	}
	
	ThisType operator*(const ThisType& other) const
	{
		return ThisType(VectorTypeTraits::mul(value, other.value));
	}
	
	ThisType operator/(const ThisType& other) const
	{
		return ThisType(VectorTypeTraits::div(value, other.value));
	}
	
	ThisType& operator+=(const ThisType& other)
	{
		value = VectorTypeTraits::add(value, other.value);
		return *this;
	}
	
	ThisType& operator-=(const ThisType& other)
	{
		value = VectorTypeTraits::sub(value, other.value);
		return *this;
	}
	
	ThisType& operator*=(const ThisType& other)
	{
		value = VectorTypeTraits::mul(value, other.value);
		return *this;
	}
	
	ThisType& operator/=(const ThisType& other)
	{
		value = VectorTypeTraits::div(value, other.value);
		return *this;
	}
	
	/*ThisType pow(const ThisType& other) const
	{
		return ThisType(VectorTypeTraits::pow(value, other.value));
	}*/
	
	ThisType pow2() const
	{
		return ThisType(VectorTypeTraits::mul(value, value));
	}
	
	ThisType sqrt() const
	{
		return ThisType(VectorTypeTraits::sqrt(value));
	}
	
	/*ThisType& pow_self(const ThisType& other)
	{
		value = VectorTypeTraits::pow(value, other.value);
		return *this;
	}*/
	
	ThisType& pow2_self()
	{
		value = VectorTypeTraits::mul(value, value);
		return *this;
	}
	
	ThisType& sqrt_self()
	{
		value = VectorTypeTraits::sqrt(value);
		return *this;
	}
	
	// returns value of self * other1 + other2
	ThisType mul_add(const ThisType& other1, const ThisType& other2) const
	{
		return ThisType(VectorTypeTraits::mul_add(value, other1.value, other2.value));
	}
	
	// calculates self := other1 * other2 + self
	ThisType& mul_add_self(const ThisType& other1, const ThisType& other2)
	{
		value = VectorTypeTraits::mul_add(other1.value, other2.value, value);
		return *this;
	}
	
	// returns value of -(self * other1) + other2
	// equivalent to other2 - (self * other1)
	ThisType neg_mul_add(const ThisType& other1, const ThisType& other2) const
	{
		return ThisType(VectorTypeTraits::neg_mul_add(value, other1.value, other2.value));
	}
	
	// calculates self := -(other1 * other2) + self
	// equivalent to self := self - (other1 * other2)
	ThisType& neg_mul_add_self(const ThisType& other1, const ThisType& other2)
	{
		value = VectorTypeTraits::neg_mul_add(other1.value, other2.value, value);
		return *this;
	}
	
	// returns sum of vector elements
	ElementType sum_elements() const
	{
		return VectorTypeTraits::sum_elements(value);
	}
	
	static ElementType* alloc(size_t element_count)
	{
		return VectorTypeTraits::alloc(element_count);
	}
	
	ElementType operator[](size_t index) const
	{
		return VectorTypeTraits::get_at(value, index);
	}
	
	HalfVectorType get_lower_half() const
	{
		return HalfVectorType(VectorTypeTraits::get_lower_half(value));
	}
	
	HalfVectorType get_upper_half() const
	{
		return HalfVectorType(VectorTypeTraits::get_upper_half(value));
	}
	
	static void free(ElementType* ptr)
	{
		VectorTypeTraits::free(ptr);
	}
	
	void print()
	{
		for (size_t n = 0; n < ElementCount; ++n)
			printf("%g ", VectorTypeTraits::get_at(value, n));
		printf("\n");
	}
	
	static size_t align_to_element_count(size_t value)
	{
		return VectorTypeTraits::align_to_element_count(value);
	}
};

#endif // _VECTOR_HPP_
