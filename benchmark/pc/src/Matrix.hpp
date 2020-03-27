/**
	Copyright (c) 2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	Matrix.hpp
	Helper Matrix class. It was designed as a replacement for double** type.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _MATRIX_HPP_
#define _MATRIX_HPP_

#include <stdint.h>
#include "DoubleVector.hpp"

class Matrix {
public:
	double* __restrict__ * __restrict__ data;

	Matrix()
		: data(NULL)
	{}

	~Matrix() {
		if (NULL != data) {
			DoubleVector::free(data[0]);
			delete[] data;
		}
	}

	void allocate(int dim) {
		if (NULL == data) {
			size_t rows = dim + 3;
			size_t column_count_aligned = DoubleVector::align_to_element_count(dim + 3);

			this->data = new double*[rows];

			double* ptr = DoubleVector::alloc(rows * column_count_aligned);

			for (size_t n = 0; n < rows; ++n) {
				this->data[n] = ptr;
				ptr += column_count_aligned;
			}
		}
	}

	double* operator[](size_t row) {
		return data[row];
	}

	const double* operator[](size_t row) const {
		return data[row];
	}
};

#endif
