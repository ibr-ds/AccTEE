/**
	Copyright (c) 2013,2016-2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	pc.cpp
	[TODO]

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <cmath>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cstdlib>
#include <ctime>
#include "Graph.hpp"
#include "utility.hpp"
#include "boinc_functions.hpp"
#include "pc.hpp"
#include "boinc_api.h"
#include "BoincFile.hpp"
#include "DoubleVector.hpp"
#include "Matrix.hpp"

#include <time.h>

static int workloadSize;
static char* workload;

#ifdef __cplusplus
extern "C" {
#endif
char* allocateWorkload(int size) {
    workload = (char*)malloc(size + 1);
    workloadSize = size;
    workload[size] = '\0';
    return workload;
}

#ifdef __cplusplus
}
#endif

class Clock
{
	clock_t clk;

public:
	explicit Clock(bool autostart = false)
	{
		if (autostart)
			start();
	}

	void start()
	{
		clk = clock();
	}

	void stop(const char* msg)
	{
		printf("%s: %.3f\n", msg, double(clock() - clk) / CLOCKS_PER_SEC);
	}

	void stop(const char* msg, int arg)
	{
		printf("%s %d: %.3f\n", msg, arg, double(clock() - clk) / CLOCKS_PER_SEC);
	}
};

#ifdef _MSC_VER
#define __attribute__(x)
#endif

using namespace std;

constexpr DoubleVector vd_1_0(1.0);

#ifdef HAS_DOUBLE_VECTOR_2
constexpr DoubleVector2 vd2_1_0(1.0);
#endif
#ifdef HAS_DOUBLE_VECTOR_4
constexpr DoubleVector4 vd4_1_0(1.0);
#endif

__attribute__((hot, always_inline)) static inline
double pow2(double val)
{
	return val * val;
}

__attribute__((hot, always_inline)) static inline
double getCorrelationCoefficient1(double a, const RHO& b, const RHO& c)
{
#ifdef HAS_DOUBLE_VECTOR_2
	DoubleVector2 v2 = DoubleVector2(&b.v) * DoubleVector2(&c.v);
	return (a - v2[0]) * v2[1];
#else
	return (a - b.v * c.v) * (b.vPrime * c.vPrime);
#endif
}

__attribute__((hot, always_inline)) static inline
double getCorrelationCoefficient1(double a, double b, double c)
{
	return (a - (b * c)) / sqrt((1 - pow2(b)) * (1 - pow2(c)));
}

__attribute__((hot, always_inline)) static inline
void getCorrelationCoefficient2(double a1, double b1, double c1, double a2, double b2, double c2, double* __restrict__ result)
{
#ifdef HAS_DOUBLE_VECTOR_2
	DoubleVector2 va(a2, a1);
	DoubleVector2 vb(b2, b1);
	DoubleVector2 vc(c2, c1);
	DoubleVector2 vres = (va - vb * vc) / ((vd2_1_0 - vb * vb) * (vd2_1_0 - vc * vc)).sqrt();
	vres.store(result);
#else
	result[0] = getCorrelationCoefficient1(a1, b1, c1);
	result[1] = getCorrelationCoefficient1(a2, b2, c2);
#endif
}

/*__attribute__((hot, always_inline)) static inline
void getCorrelationCoefficient4(double a1, double b1, double c1, double a2, double b2, double c2,
		double a3, double b3, double c3, double a4, double b4, double c4, double* __restrict__ result)
{
#ifdef HAS_DOUBLE_VECTOR_4
	DoubleVector4 va(a4, a3, a2, a1);
	DoubleVector4 vb(b4, b3, b2, b1);
	DoubleVector4 vc(c4, c3, c2, c1);
	DoubleVector4 vres = (va - vb * vc) / ((vd4_1_0 - vb * vb) * (vd4_1_0 - vc * vc)).sqrt();
	vres.store(result);
#else
	getCorrelationCoefficient2(a1, b1, c1, a2, b2, c2, result);
	getCorrelationCoefficient2(a3, b3, c3, a4, b4, c4, result + 2);
#endif
}*/

// Previous version of correlation coefficient code, left here for reference.
// Note: this algorithm version probably will be more friendly for GPU than Gray version.
#if 0
/* Load data needed for correlation coefficient computing. This part is executed once, so it will be outside of template */
__attribute__((hot, always_inline)) static inline
void loadDataForCorrelations(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
	const int* __restrict__ subset, const int l, double * __restrict__ * __restrict__ p) {
	int dim = l + 2;
	double* tmpData = p[dim];

	// initialization of p (looks like rho)
	for (int j = 1; j < dim; j++) {
		int second;

		if (j == 1) {
			second = b;
		} else {
			second = neighbours[subset[j - 2]];
		}

		for (int i = 0; i < j; i++) {
			int first;
			if (i == 0) {
				first = a;
			} else if (i == 1) {
				first = b;
			} else {
				first = neighbours[subset[i - 2]];
			}

			p[j][i] = g->rho[first][second].v;

			if (j == dim - 1) {
				tmpData[i] = g->rho[first][second].vPrime;
			}
		}
	}
}

/* Templated version of correlations(). L and K values are constant, so compiler can apply
 * more efficient optimizations here.
 */
template<unsigned int K, unsigned int L, bool Last = K == L-1/*+1*/>
struct Correlations
{
	__attribute__((hot, always_inline)) inline
	static double calc(double * __restrict__ * __restrict__ p)
	{
		int dim = L + 2;
		double* tmpData = p[dim];

		if (K > 1) {
			for (unsigned int z = 0; z <= (dim - K); z += DoubleVector::ElementCount) {
				//tmpData[z] = 1.0 / sqrt(1.0 - pow2(p[dim - K][z]));

				DoubleVector v(&p[dim - K][z]);
				v = vd_1_0 / (vd_1_0 - v * v).sqrt();
				v.store(&tmpData[z]);
			}
		}

		for (unsigned int j = 1; j < (dim - K); j++) {
			DoubleVector v_p_j_dim_SUB_k(p[dim - K][j]);
			DoubleVector v_tmpData_j(tmpData[j]);

			for (unsigned int i = 0; i < j; i += DoubleVector::ElementCount) {
				//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

				DoubleVector v_A(&p[j][i]);
				DoubleVector v_B(&p[dim - K][i]);
				DoubleVector v_tmpData_i(&tmpData[i]);

				DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
				v.store(&p[j][i]);
			}
		}

		return Correlations<K+1, L>::calc(p);
	}
};

template<unsigned int K, unsigned int L>
struct Correlations<K, L, true>
{
	__attribute__((hot, always_inline)) inline
	static double calc(double * __restrict__ * __restrict__ p)
	{
		double result[2] __attribute__((aligned(16)));
		getCorrelationCoefficient2(p[1][0], p[3][0], p[3][1], p[2][0], p[3][0], p[3][2], result);
		double cc_1_0 = result[0];
		double cc_2_0 = result[1];
		double cc_2_1 = getCorrelationCoefficient1(p[2][1], p[3][1], p[3][2]);


		return getCorrelationCoefficient1(cc_1_0, cc_2_0, cc_2_1);
	}
};

template<unsigned int L>
struct GetCorrelation
{
	__attribute__((hot, always_inline)) inline
	static double calc(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
		const int* __restrict__ subset, double * __restrict__ * __restrict__ p)
	{
		loadDataForCorrelations(a, b, g, neighbours, subset, (int)L, p);

		return Correlations<1, L>::calc(p);
	}
};


/** Finds the correlation coefficient when l is greater than 1 (formally, when l > 1).

	@param const int a
	Index of the selected row, that also represents the "departure" node i of an edge i->j.

	@param const int b
	Index of the selected column, that also represents the "arrival" node j of an edge i->j.

	@param const Graph* g
	The Graph object representing the gene network.

	@param const int* neighbours
	Set of the nearby nodes.

	@param const int* subset
	Subset of the lookup indexes of the nearby nodes.
	Note that this subset has cardinality l.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param double ** p
	Rho value.

	@return The decimal value of the computed correlation for the edge a->b depending on the given neighbours' subset.
*/
__attribute__((hot, always_inline)) static inline
double correlations(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
	const int* __restrict__ subset, const int l, double * __restrict__ * __restrict__ p) {
	if (4 == l) {
		return GetCorrelation<4>::calc(a, b, g, neighbours, subset, p);
	}
	if (5 == l) {
		return GetCorrelation<5>::calc(a, b, g, neighbours, subset, p);
	}
	if (6 == l) {
		return GetCorrelation<6>::calc(a, b, g, neighbours, subset, p);
	}
	if (7 == l) {
		return GetCorrelation<7>::calc(a, b, g, neighbours, subset, p);
	}
	if (8 == l) {
		return GetCorrelation<8>::calc(a, b, g, neighbours, subset, p);
	}
	if (9 == l) {
		return GetCorrelation<9>::calc(a, b, g, neighbours, subset, p);
	}
	if (10 == l) {
		return GetCorrelation<10>::calc(a, b, g, neighbours, subset, p);
	}
	if (11 == l) {
		return GetCorrelation<11>::calc(a, b, g, neighbours, subset, p);
	}


	int dim = l + 2;
	double* tmpData = p[dim];

	// initialization of p (looks like rho)
	for (int j = 1; j < dim; j++) {
		int second;

		if (j == 1) {
			second = b;
		} else {
			second = neighbours[subset[j - 2]];
		}

		for (int i = 0; i < j; i++) {
			int first;
			if (i == 0) {
				first = a;
			} else if (i == 1) {
				first = b;
			} else {
				first = neighbours[subset[i - 2]];
			}

			p[j][i] = g->rho[first][second].v;

			if (j == dim - 1) {
				tmpData[i] = g->rho[first][second].vPrime;
			}
		}
	}

	for (int k = 1; k <= l; k++) {
		if (k > 1) {
			for (int z = 0; z <= (dim - k); z += DoubleVector::ElementCount) {
				//tmpData[z] = 1.0 / sqrt(1.0 - pow2(p[dim - K][z]));

				DoubleVector v(&p[dim - k][z]);
				v = vd_1_0 / (vd_1_0 - v * v).sqrt();
				v.store(&tmpData[z]);
			}
		}

		for (int j = 1; j < (dim - k); j++) {
			DoubleVector v_p_j_dim_SUB_k(p[dim - k][j]);
			DoubleVector v_tmpData_j(tmpData[j]);

			for (int i = 0; i < j; i += DoubleVector::ElementCount) {
				//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

				DoubleVector v_A(&p[j][i]);
				DoubleVector v_B(&p[dim - k][i]);
				DoubleVector v_tmpData_i(&tmpData[i]);

				DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
				v.store(&p[j][i]);
			}
		}
	}

	return p[1][0];
}
#endif


/** Finds the correlation coefficient when l is greater than 1 (formally, when l > 1).
	This algorithm version is optimized for use with Gray combination generator.
	This function version performs all calculations and stores all intermediate results.

	@param const int a
	Index of the selected row, that also represents the "departure" node i of an edge i->j.

	@param const int b
	Index of the selected column, that also represents the "arrival" node j of an edge i->j.

	@param const Graph* g
	The Graph object representing the gene network.

	@param const int* neighbours
	Set of the nearby nodes.

	@param const int* subset
	Subset of the lookup indexes of the nearby nodes.
	Note that this subset has cardinality l.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param Matrix* matrices
	Rho matrices.

	@return The decimal value of the computed correlation for the edge a->b depending on the given neighbours' subset.
*/
__attribute__((hot, always_inline)) static inline
double correlationsGrayFirst(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
	const int* __restrict__ subset, const int l, Matrix* __restrict__ matrices) {
	double * __restrict__ * __restrict__ p = matrices[l].data;
	double * __restrict__ * __restrict__ next_p = matrices[l - 1].data;

	int dim = l + 2;
	double* tmpData = p[dim];

	// initialization of p (looks like rho)
	for (int j = 1; j < dim; j++) {
		int second;

		if (j == 1) {
			second = b;
		} else {
			second = neighbours[subset[j - 2]];
		}

		for (int i = 0; i < j; i++) {
			int first;
			if (i == 0) {
				first = a;
			} else if (i == 1) {
				first = b;
			} else {
				first = neighbours[subset[i - 2]];
			}

			p[j][i] = g->rho[first][second].v;

			if (j == dim - 1) {
				tmpData[i] = g->rho[first][second].vPrime;
			}
		}
	}

	for (int k = 1; k <= l; k++) {
		if (k > 1) {
			p = next_p;
			next_p = matrices[l - k].data;
			tmpData = p[dim - k + 1];

			for (int z = 0; z <= (dim - k); z += DoubleVector::ElementCount) {
				//tmpData[z] = 1.0 / sqrt(1.0 - pow2(p[dim - K][z]));

				DoubleVector v(&p[dim - k][z]);
				v = vd_1_0 / (vd_1_0 - v * v).sqrt();
				v.store(&tmpData[z]);
			}
		}

		for (int j = 1; j < (dim - k); j++) {
			DoubleVector v_p_j_dim_SUB_k(p[dim - k][j]);
			DoubleVector v_tmpData_j(tmpData[j]);

			for (int i = 0; i < j; i += DoubleVector::ElementCount) {
				//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

				DoubleVector v_A(&p[j][i]);
				DoubleVector v_B(&p[dim - k][i]);
				DoubleVector v_tmpData_i(&tmpData[i]);

				DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
				v.store(&next_p[j][i]);
			}
		}
	}

	return next_p[1][0];
}

/* Load data needed for correlation coefficient computing. This part is executed once, so it will be outside of template */
__attribute__((hot, always_inline)) static inline
void loadDataForCorrelationsGrayNext(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
	const int* __restrict__ subset, const int l, Matrix* __restrict__ matrices, const int changedValueIndex) {

	double * __restrict__ * __restrict__ p = matrices[l].data;

	int dim = l + 2;
	double* tmpData = p[dim];

	// initialization of p (looks like rho)
	int first, second, i, j;

	// update change row
	j = changedValueIndex + 2;
	second = neighbours[subset[changedValueIndex]];
	for (i = 0; i < j; i++) {
		if (i == 0) {
			first = a;
		} else if (i == 1) {
			first = b;
		} else {
			first = neighbours[subset[i - 2]];
		}

		p[j][i] = g->rho[first][second].v;

		if (j == dim - 1) {
			tmpData[i] = g->rho[first][second].vPrime;
		}
	}

	// update changed column
	i = changedValueIndex + 2;
	first = neighbours[subset[changedValueIndex]];
	for (j = i + 1; j < dim; j++) {
		second = neighbours[subset[j - 2]];

		p[j][i] = g->rho[first][second].v;

		if (j == dim - 1) {
			tmpData[i] = g->rho[first][second].vPrime;
		}
	}
}

/* Templated version of correlations(). L and K values are constant, so compiler can apply
 * more efficient optimizations here.
 */
template<unsigned int K, unsigned int L, bool Last = K == L-1/*+1*/>
struct CorrelationsGrayNext
{
	__attribute__((hot, always_inline)) inline
	static double calc(Matrix* __restrict__ matrices, const int changedValueIndex)
	{
		double * __restrict__ * __restrict__ p = matrices[L - K + 1].data;
		double * __restrict__ * __restrict__ next_p = matrices[L - K].data;

		int dim = L + 2;
		unsigned int changedIndex = changedValueIndex + 2;
		double* tmpData = p[dim - K + 1];

		// initialization of p (looks like rho)
		unsigned int i, j;

		if (K > 1) {
			if (changedIndex < dim - K) {
				tmpData[changedIndex] = 1.0 / sqrt(1.0 - pow2(p[dim - K][changedIndex]));
			} else {
				for (unsigned int z = 0; z <= (dim - K); z += DoubleVector::ElementCount) {
					//tmpData[z] = 1.0 / sqrt(1.0 - pow2(p[dim - K][z]));

					DoubleVector v(&p[dim - K][z]);
					v = vd_1_0 / (vd_1_0 - v * v).sqrt();
					v.store(&tmpData[z]);
				}
			}
		}

		if (changedIndex < dim - K) {
			// update changed row
			j = changedIndex;
			DoubleVector v_p_j_dim_SUB_k(p[dim - K][j]);
			DoubleVector v_tmpData_j(tmpData[j]);

			for (i = 0; i < j; i += DoubleVector::ElementCount) {
				//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

				DoubleVector v_A(&p[j][i]);
				DoubleVector v_B(&p[dim - K][i]);
				DoubleVector v_tmpData_i(&tmpData[i]);

				DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
				v.store(&next_p[j][i]);
			}

			// update changed column
			i = changedIndex;
			double p_i_dim_SUB_k = p[dim - K][i];
			double tmpData_i = tmpData[i];
			for (j = i + 1; j < (dim - K); j++) {
				next_p[j][i] = (p[j][i] - p[dim - K][j] * p_i_dim_SUB_k) * (tmpData_i * tmpData[j]);
			}
		} else {
			for (j = 1; j < (dim - K); j++) {
				DoubleVector v_p_j_dim_SUB_k(p[dim - K][j]);
				DoubleVector v_tmpData_j(tmpData[j]);

				for (i = 0; i < j; i += DoubleVector::ElementCount) {
					//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

					DoubleVector v_A(&p[j][i]);
					DoubleVector v_B(&p[dim - K][i]);
					DoubleVector v_tmpData_i(&tmpData[i]);

					DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
					v.store(&next_p[j][i]);
				}
			}
		}

		return CorrelationsGrayNext<K+1, L>::calc(matrices, changedValueIndex);
	}
};

template<unsigned int K, unsigned int L>
struct CorrelationsGrayNext<K, L, true>
{
	__attribute__((hot, always_inline)) inline
	static double calc(Matrix* __restrict__ matrices, const int /*changedValueIndex*/)
	{
		double * __restrict__ * __restrict__ p = matrices[L - K + 1].data;

		double result[2] __attribute__((aligned(16)));
		getCorrelationCoefficient2(p[1][0], p[3][0], p[3][1], p[2][0], p[3][0], p[3][2], result);
		double cc_1_0 = result[0];
		double cc_2_0 = result[1];
		double cc_2_1 = getCorrelationCoefficient1(p[2][1], p[3][1], p[3][2]);


		return getCorrelationCoefficient1(cc_1_0, cc_2_0, cc_2_1);
	}
};

template<unsigned int L>
struct GetCorrelationGrayNext
{
	__attribute__((hot, always_inline)) inline
	static double calc(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
		const int* __restrict__ subset, Matrix* __restrict__ matrices, const int changedValueIndex)
	{
		loadDataForCorrelationsGrayNext(a, b, g, neighbours, subset, (int)L, matrices, changedValueIndex);

		return CorrelationsGrayNext<1, L>::calc(matrices, changedValueIndex);
	}
};

/** Finds the correlation coefficient when l is greater than 1 (formally, when l > 1).
	This algorithm version is optimized for use with Gray combination generator.
	It recalculates only values which depends on changed combination element.

	@param const int a
	Index of the selected row, that also represents the "departure" node i of an edge i->j.

	@param const int b
	Index of the selected column, that also represents the "arrival" node j of an edge i->j.

	@param const Graph* g
	The Graph object representing the gene network.

	@param const int* neighbours
	Set of the nearby nodes.

	@param const int* subset
	Subset of the lookup indexes of the nearby nodes.
	Note that this subset has cardinality l.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param Matrix* matrices
	Rho matrices.

	@return The decimal value of the computed correlation for the edge a->b depending on the given neighbours' subset.
*/
__attribute__((hot, always_inline)) static inline
double correlationsGrayNext(const int a, const int b, const Graph* __restrict__ g, const int* __restrict__ neighbours,
	const int* __restrict__ subset, const int l, Matrix* __restrict__ matrices, const int changedValueIndex) {
	if (4 == l) {
		return GetCorrelationGrayNext<4>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (5 == l) {
		return GetCorrelationGrayNext<5>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (6 == l) {
		return GetCorrelationGrayNext<6>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (7 == l) {
		return GetCorrelationGrayNext<7>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (8 == l) {
		return GetCorrelationGrayNext<8>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (9 == l) {
		return GetCorrelationGrayNext<9>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (10 == l) {
		return GetCorrelationGrayNext<10>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}
	if (11 == l) {
		return GetCorrelationGrayNext<11>::calc(a, b, g, neighbours, subset, matrices, changedValueIndex);
	}

	double * __restrict__ * __restrict__ p = matrices[l].data;
	double * __restrict__ * __restrict__ next_p = matrices[l - 1].data;

	int dim = l + 2;
	int changedIndex = changedValueIndex + 2;
	double* tmpData = p[dim];

	// initialization of p (looks like rho)
	int first, second, i, j;

	// update change row
	j = changedValueIndex + 2;
	second = neighbours[subset[changedValueIndex]];
	for (i = 0; i < j; i++) {
		if (i == 0) {
			first = a;
		} else if (i == 1) {
			first = b;
		} else {
			first = neighbours[subset[i - 2]];
		}

		p[j][i] = g->rho[first][second].v;

		if (j == dim - 1) {
			tmpData[i] = g->rho[first][second].vPrime;
		}
	}

	// update changed column
	i = changedValueIndex + 2;
	first = neighbours[subset[changedValueIndex]];
	for (j = i + 1; j < dim; j++) {
		second = neighbours[subset[j - 2]];

		p[j][i] = g->rho[first][second].v;

		if (j == dim - 1) {
			tmpData[i] = g->rho[first][second].vPrime;
		}
	}


	for (int k = 1; k <= l; k++) {
		if (k > 1) {
			p = next_p;
			next_p = matrices[l - k].data;
			tmpData = p[dim - k + 1];

			if (changedIndex < dim - k) {
				tmpData[changedIndex] = 1.0 / sqrt(1.0 - pow2(p[dim - k][changedIndex]));
			} else {
				for (int z = 0; z <= (dim - k); z += DoubleVector::ElementCount) {
					//tmpData[z] = 1.0 / sqrt(1.0 - pow2(p[dim - K][z]));

					DoubleVector v(&p[dim - k][z]);
					v = vd_1_0 / (vd_1_0 - v * v).sqrt();
					v.store(&tmpData[z]);
				}
			}
		}

		if (changedIndex < dim - k) {
			// update changed row
			j = changedIndex;
			DoubleVector v_p_j_dim_SUB_k(p[dim - k][j]);
			DoubleVector v_tmpData_j(tmpData[j]);

			for (i = 0; i < j; i += DoubleVector::ElementCount) {
				//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

				DoubleVector v_A(&p[j][i]);
				DoubleVector v_B(&p[dim - k][i]);
				DoubleVector v_tmpData_i(&tmpData[i]);

				DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
				v.store(&next_p[j][i]);
			}

			// update changed column
			i = changedIndex;
			double p_i_dim_SUB_k = p[dim - k][i];
			double tmpData_i = tmpData[i];
			for (j = i + 1; j < (dim - k); j++) {
				next_p[j][i] = (p[j][i] - p[dim - k][j] * p_i_dim_SUB_k) * (tmpData_i * tmpData[j]);
			}
		} else {
			for (j = 1; j < (dim - k); j++) {
				DoubleVector v_p_j_dim_SUB_k(p[dim - k][j]);
				DoubleVector v_tmpData_j(tmpData[j]);

				for (i = 0; i < j; i += DoubleVector::ElementCount) {
					//p[j][i] = (p[j][i] - p_j_dim_SUB_k * p[dim - K][i]) * (tmpData[i] * tmpData_j);

					DoubleVector v_A(&p[j][i]);
					DoubleVector v_B(&p[dim - k][i]);
					DoubleVector v_tmpData_i(&tmpData[i]);

					DoubleVector v = (v_A - v_p_j_dim_SUB_k * v_B) * (v_tmpData_i * v_tmpData_j);
					v.store(&next_p[j][i]);
				}
			}
		}
	}

	return next_p[1][0];
}

/** Tests the causality and, when appropriate, cuts both the edges r->c and c->r.

	Tests the causality w.r.t. the correlation coefficient calculating the probability value of causal dependency.
	Whether this value is greater than the alpha value (complement of confidence interval), both the edges r->c
	and c->r are cutted.

	@param double correlationCoefficient
	Correlation coefficient of the edge r->c.

	@param Graph* &g
	The reference of the Graph object representing the gene network.

	@param const int r
	Index of the selected row, that also represents the node r of an edge r->c.

	@param const int c
	Index of the selected column, that also represents the node c of an edge r->c.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param const double alpha
	Complement of the confidence interval.

	@return nothing.
*/

__attribute__((hot, always_inline)) static inline
bool IsNan(double val) {
#ifdef _MSC_VER
	return _isnan(val);
#else
	return std::isnan(val);
#endif
}

__attribute__((hot, always_inline)) static inline
bool testAndRemove(double correlationCoefficient, Graph* __restrict__ g, const int r, const int c, const double ccRangeMin, const double ccRangeMax) {
	++g->score;

	if (IsNan(correlationCoefficient) || ((ccRangeMin <= correlationCoefficient) && (correlationCoefficient <= ccRangeMax))) {
		// remove arcs
		g->matrix[r][c] = g->matrix[c][r] = false;

		// decrement neighbours
		g->numNeighbours[r]--;
		g->numNeighbours[c]--;

		return true;
	}

	return false;
}

// Another unused function, may be useful for OpenCL app
#if 0
/** Computes the correlation coefficient related to the edge r->c considering
	subsets of cardinality l of neaighbour edges.

	@param int* neighbours
	Set of the indexes of the nearby nodes of the given edge i->j

	@param const int* subset
	Subset of the lookup indexes of the nearby nodes.
	Note that this subset has cardinality l.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param Graph* &g
	The reference of the Graph object representing the gene network.

	@param const int r
	Index of the selected row, that also represents the node r of an edge r->c.

	@param const int c
	Index of the selected column, that also represents the node c of an edge r->c.

	@param double** p
	Rho.

	@return The coefficient of correlation.
*/
__attribute__((hot, always_inline)) static inline
double getCorrelationCoefficient(const int* __restrict__ neighbours, const int* __restrict__ subset, /*const int l*,*/
	Graph* __restrict__ g, const int r, const int c/*, double* __restrict__ * __restrict__ p*/) {
	double correlationCoefficient;

	// This function is used for L=2 only, so unused parts of code are commented out.

	if (l == 2) {
		double rhoRC, rhoRH1, rhoCH1;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];

		rhoRC = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h2], g->rho[c][h2]);
		rhoRH1 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h2], g->rho[h1][h2]);
		rhoCH1 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h2], g->rho[h1][h2]);


		correlationCoefficient = getCorrelationCoefficient1(rhoRC, rhoRH1, rhoCH1);
	} else if (l == 3) {
		double rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3, rhoRC_H3, rhoRH1_H3, rhoRH2_H3, rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];
		int h3 = neighbours[subset[2]];


		rhoRC_H3 = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h3], g->rho[c][h3]);
		rhoRH1_H3 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h3], g->rho[h1][h3]);
		rhoRH2_H3 = getCorrelationCoefficient1(g->rho[r][h2].v, g->rho[r][h3], g->rho[h2][h3]);
		rhoCH1_H3 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h3], g->rho[h1][h3]);
		rhoCH2_H3 = getCorrelationCoefficient1(g->rho[c][h2].v, g->rho[c][h3], g->rho[h2][h3]);
		rhoH1H2_H3 = getCorrelationCoefficient1(g->rho[h1][h2].v, g->rho[h1][h3], g->rho[h2][h3]);


		double result[2] __attribute__((aligned(16)));
		getCorrelationCoefficient2(rhoRC_H3, rhoRH2_H3, rhoCH2_H3, rhoRH1_H3, rhoRH2_H3, rhoH1H2_H3, result);
		rhoRC_H2H3 = result[0];
		rhoRH1_H2H3 = result[1];
		rhoCH1_H2H3 = getCorrelationCoefficient1(rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3);


		correlationCoefficient = getCorrelationCoefficient1(rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3);
	} else {
		correlationCoefficient = correlations(r, c, g, neighbours, subset, l, p);
	}

	return correlationCoefficient;
}
#endif

__attribute__((hot, always_inline)) static inline
double getCorrelationCoefficientGrayFirst(const int* __restrict__ neighbours, const int* __restrict__ subset, const int l,
	Graph* __restrict__ g, const int r, const int c, Matrix* __restrict__ matrices) {
	if (l == 2) {
		double rhoRC, rhoRH1, rhoCH1;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];

		rhoRC = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h2], g->rho[c][h2]);
		rhoRH1 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h2], g->rho[h1][h2]);
		rhoCH1 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h2], g->rho[h1][h2]);

		double* tmpVal = matrices[2].data[0];
		tmpVal[0] = rhoRC;

		return getCorrelationCoefficient1(rhoRC, rhoRH1, rhoCH1);
	}
	else if (l == 3) {
		double rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3, rhoRC_H3, rhoRH1_H3, rhoRH2_H3, rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];
		int h3 = neighbours[subset[2]];


		rhoRC_H3 = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h3], g->rho[c][h3]); // 2; 1t
		rhoRH1_H3 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h3], g->rho[h1][h3]); // 0, 2; 1t
		rhoRH2_H3 = getCorrelationCoefficient1(g->rho[r][h2].v, g->rho[r][h3], g->rho[h2][h3]); // 1, 2; 0t
		rhoCH1_H3 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h3], g->rho[h1][h3]); // 0, 2; 1t
		rhoCH2_H3 = getCorrelationCoefficient1(g->rho[c][h2].v, g->rho[c][h3], g->rho[h2][h3]); // 1, 2; 0t
		rhoH1H2_H3 = getCorrelationCoefficient1(g->rho[h1][h2].v, g->rho[h1][h3], g->rho[h2][h3]); // 0, 1, 2

		double* tmpVal = matrices[3].data[0];
		tmpVal[0] = rhoRC_H3;
		tmpVal[1] = rhoRH1_H3;
		tmpVal[2] = rhoRH2_H3;
		tmpVal[3] = rhoCH1_H3;
		tmpVal[4] = rhoCH2_H3;

		double result[2] __attribute__((aligned(16)));
		getCorrelationCoefficient2(rhoRC_H3, rhoRH2_H3, rhoCH2_H3, rhoRH1_H3, rhoRH2_H3, rhoH1H2_H3, result); // 1, 2; 0t | 0, 1, 2
		rhoRC_H2H3 = result[0];
		rhoRH1_H2H3 = result[1];
		rhoCH1_H2H3 = getCorrelationCoefficient1(rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3); // 0, 1, 2

		tmpVal[5] = rhoRC_H2H3;

		return getCorrelationCoefficient1(rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3);
	} else
		return correlationsGrayFirst(r, c, g, neighbours, subset, l, matrices);
}

__attribute__((hot, always_inline)) static inline
double getCorrelationCoefficientGrayNext(const int* __restrict__ neighbours, const int* __restrict__ subset, const int l,
	Graph* __restrict__ g, const int r, const int c, Matrix* __restrict__ matrices, const int changedValueIndex) {
	if (l == 2) {
		double rhoRC, rhoRH1, rhoCH1;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];

		double* tmpVal = matrices[2].data[0];

		if (0 == changedValueIndex)
			rhoRC = tmpVal[0];
		else
			tmpVal[0] = rhoRC = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h2], g->rho[c][h2]);
		rhoRH1 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h2], g->rho[h1][h2]);
		rhoCH1 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h2], g->rho[h1][h2]);

		return getCorrelationCoefficient1(rhoRC, rhoRH1, rhoCH1);
	}
	else if (l == 3) {
		double* tmpVal = matrices[3].data[0];
		double rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3, rhoRC_H3, rhoRH1_H3, rhoRH2_H3, rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3;
		int h1 = neighbours[subset[0]];
		int h2 = neighbours[subset[1]];
		int h3 = neighbours[subset[2]];

		double result[2] __attribute__((aligned(16)));

		if (0 == changedValueIndex) {
			rhoRH1_H3 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h3], g->rho[h1][h3]); // 0, 2; 1t
			rhoRH2_H3 = tmpVal[2]; // 1, 2; 0t
			rhoCH1_H3 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h3], g->rho[h1][h3]); // 0, 2; 1t
			rhoCH2_H3 = tmpVal[4]; // 1, 2; 0t
			rhoH1H2_H3 = getCorrelationCoefficient1(g->rho[h1][h2].v, g->rho[h1][h3], g->rho[h2][h3]); // 0, 1, 2

			tmpVal[1] = rhoRH1_H3;
			tmpVal[3] = rhoCH1_H3;

			rhoRC_H2H3 = tmpVal[5];
			getCorrelationCoefficient2(rhoRH1_H3, rhoRH2_H3, rhoH1H2_H3, rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3, result); // 1, 2; 0t | 0, 1, 2
			rhoRH1_H2H3 = result[0];
			rhoCH1_H2H3 = result[1];

			return getCorrelationCoefficient1(rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3);
		} else if (1 == changedValueIndex) {
			rhoRC_H3 = tmpVal[0]; // 2; 1t
			rhoRH1_H3 = tmpVal[1]; // 0, 2; 1t
			rhoRH2_H3 = getCorrelationCoefficient1(g->rho[r][h2].v, g->rho[r][h3], g->rho[h2][h3]); // 1, 2; 0t
			rhoCH1_H3 = tmpVal[3]; // 0, 2; 1t
			rhoCH2_H3 = getCorrelationCoefficient1(g->rho[c][h2].v, g->rho[c][h3], g->rho[h2][h3]); // 1, 2; 0t
			rhoH1H2_H3 = getCorrelationCoefficient1(g->rho[h1][h2].v, g->rho[h1][h3], g->rho[h2][h3]); // 0, 1, 2

			tmpVal[2] = rhoRH2_H3;
			tmpVal[4] = rhoCH2_H3;

			getCorrelationCoefficient2(rhoRC_H3, rhoRH2_H3, rhoCH2_H3, rhoRH1_H3, rhoRH2_H3, rhoH1H2_H3, result); // 1, 2; 0t | 0, 1, 2
			rhoRC_H2H3 = result[0];
			rhoRH1_H2H3 = result[1];
			rhoCH1_H2H3 = getCorrelationCoefficient1(rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3); // 0, 1, 2

			tmpVal[5] = rhoRC_H2H3;

			return getCorrelationCoefficient1(rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3);
		} else {
			rhoRC_H3 = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h3], g->rho[c][h3]); // 2; 1t
			rhoRH1_H3 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h3], g->rho[h1][h3]); // 0, 2; 1t
			rhoRH2_H3 = getCorrelationCoefficient1(g->rho[r][h2].v, g->rho[r][h3], g->rho[h2][h3]); // 1, 2; 0t
			rhoCH1_H3 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h3], g->rho[h1][h3]); // 0, 2; 1t
			rhoCH2_H3 = getCorrelationCoefficient1(g->rho[c][h2].v, g->rho[c][h3], g->rho[h2][h3]); // 1, 2; 0t
			rhoH1H2_H3 = getCorrelationCoefficient1(g->rho[h1][h2].v, g->rho[h1][h3], g->rho[h2][h3]); // 0, 1, 2

			tmpVal[0] = rhoRC_H3;
			tmpVal[1] = rhoRH1_H3;
			tmpVal[2] = rhoRH2_H3;
			tmpVal[3] = rhoCH1_H3;
			tmpVal[4] = rhoCH2_H3;

			getCorrelationCoefficient2(rhoRC_H3, rhoRH2_H3, rhoCH2_H3, rhoRH1_H3, rhoRH2_H3, rhoH1H2_H3, result); // 1, 2; 0t | 0, 1, 2
			rhoRC_H2H3 = result[0];
			rhoRH1_H2H3 = result[1];
			rhoCH1_H2H3 = getCorrelationCoefficient1(rhoCH1_H3, rhoCH2_H3, rhoH1H2_H3); // 0, 1, 2

			tmpVal[5] = rhoRC_H2H3;

			return getCorrelationCoefficient1(rhoRC_H2H3, rhoRH1_H2H3, rhoCH1_H2H3);
		}
	} else
		return correlationsGrayNext(r, c, g, neighbours, subset, l, matrices, changedValueIndex);
}

// Previous version of combinations generator, may be useful for OpenCL app
#if 0
/** [TODO]

	@param int* neighbours
	Set of the indexes of the nearby nodes of the given edge i->j.

	@param const int neighboursDim
	The length of the array of nwighbours.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param Graph* &g
	The reference of the Graph object representing the gene network.

	@param const int r
	Index of the selected row, that also represents the node r of an edge r->c.

	@param const int c
	Index of the selected column, that also represents the node c of an edge r->c.

	@param const double alpha
	Complement of the confidence interval.

	@param double** p
	Rho.

	@return nothing.

	@see Thanks to Ilya Bursov, http://stackoverflow.com/questions/19327847/n-choose-k-for-large-n-and-k
*/
__attribute__((hot, always_inline)) static inline
void iterativeComb(int* __restrict__ neighbours, const int neighboursDim, const int l, Graph* __restrict__ g, const int r,
	const int c, const double ccRangeMin, const double ccRangeMax, int* __restrict__ currentCombination) {
	double coeff;

	for (int i = 0; i < neighboursDim; i++) {
		currentCombination[i] = i;
	}

	currentCombination[l - 1] = l - 1 - 1;

	do {
		if (currentCombination[l - 1] == (neighboursDim - 1)) {
			int i = l - 1 - 1;

			while (currentCombination[i] == (neighboursDim - l + i)) {
				i--;
			}

			currentCombination[i]++;

			for (int j = i + 1; j < l; j++) {
				currentCombination[j] = currentCombination[i] + j - i;
			}
		} else {
			currentCombination[l - 1]++;
		}

		coeff = getCorrelationCoefficient(neighbours, currentCombination, g, r, c);
		if (testAndRemove(coeff, g, r, c, ccRangeMin, ccRangeMax))
			return;
	} while (!((currentCombination[0] == (neighboursDim - 1 - l + 1)) && (currentCombination[l - 1] == (neighboursDim - 1))));
}
#endif

enum GrayCombMode
{
    GenLeft = 0,
    NegLeft = 1,
    GenRight = 2,
    NegRight = 3,

	NegMask = 1,
	RightMask = 2,
};

struct GrayStackElement
{
    int n;
    int k;
    GrayCombMode mode;
};

/** Combinations generator, Gray code version.
	This function implements [H-Trans] algorithm described in "Combinatorial Generation"
	by Frank Ruskey (page 129, algorithm 5.8). http://www.1stworks.com/ref/ruskeycombgen.pdf
	Code below is an iterative version of original code. It also has single place where
	value swapping is made.

	@param int* neighbours
	Set of the indexes of the nearby nodes of the given edge i->j.

	@param const int neighboursDim
	The length of the array of nwighbours.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param Graph* &g
	The reference of the Graph object representing the gene network.

	@param const int r
	Index of the selected row, that also represents the node r of an edge r->c.

	@param const int c
	Index of the selected column, that also represents the node c of an edge r->c.

	@param double ccRangeMin

	@param double ccRangeMax

	@param int* currentCombination

	@param Matrix* matrices
	Rhos matrices.

	@param GrayStackElement* grayStack

	@return nothing.

	@see Thanks to Frank Ruskey's "Combinatorial Generation", http://www.1stworks.com/ref/ruskeycombgen.pdf
*/
__attribute__((hot, always_inline)) static inline
void iterativeCombGray(int* __restrict__ neighbours, const int neighboursDim, const int l, Graph* __restrict__ g, const int r,
	const int c, const double ccRangeMin, const double ccRangeMax, int* __restrict__ currentCombination, Matrix* __restrict__ matrices,
	GrayStackElement* __restrict__ grayStack) {
	unsigned int stackTop = 0;
	bool gen = true;
	int changedPos = 0, changedVal = 0;
	int i = 0;

	// create first combination
	for (int i = 0; i < l; i++) {
		currentCombination[i] = i;
	}

	int n = neighboursDim;
	int k = l;

	// first combination needs full processing
	double coeff = getCorrelationCoefficientGrayFirst(neighbours, currentCombination, l, g, r, c, matrices);
	if (testAndRemove(coeff, g, r, c, ccRangeMin, ccRangeMax))
		return;

	while((stackTop > 0) || ((1 <= k) && (k < n))) {
		if ((1 < k) && (k < n)) {
			// push node on stack
			grayStack[stackTop].n = n;
			grayStack[stackTop].k = k;
			grayStack[stackTop].mode = gen ? GenLeft : NegLeft;
			++stackTop;

			// set node to left child
			/*if (gen) {
				// gen(n-1, k);
				--n;
			} else {
				// neg(n-2, k-2);
				n -= 2;
				k -= 2;
			}*/
			n -= gen ? 1 : 2;
			k -= gen ? 0 : 2;

			continue;
		} else if (k == 1) {
			if (0 == i) { // new loop
				i = gen ? 1 : n - 1;
			} else {
				i += gen ? 1 : -1;
			}

			if (gen) {
				if (i <= n - 1) {
					changedPos = 0;
					changedVal = i;
				} else {
					// exit loop
					i = 0;
					k = 0;
					continue;
				}
			} else {
				if (i >= 1) {
					changedPos = 0;
					changedVal = i - 1;
				} else {
					// exit loop
					i = 0;
					k = 0;
					continue;
				}
			}
		} else {
			// pop node from stack
			--stackTop;
			n = grayStack[stackTop].n;
			k = grayStack[stackTop].k;
			GrayCombMode mode = grayStack[stackTop].mode;

			// visit node and then set node to appropriate child
			switch(mode) {
				case GenLeft: {
					changedPos = k - 1;
					changedVal = n - 1;

					// keep current node on stack, update its mode only
					grayStack[stackTop].mode = GenRight;
					++stackTop;

					// set current node to middle child
					// neg(n-2, k-1);
					n -= 2;
					k -= 1;
					gen = false;
				}
				break;

				case GenRight: {
					changedPos = k - 2;
					changedVal = n - 2;

					// set current node to right child
					// gen(n-2, k-2);
					n -= 2;
					k -= 2;
					gen = true;
				}
				break;

				case NegLeft: {
					changedPos = k - 2;
					changedVal = k - 2;

					// keep current node on stack, update its mode only
					grayStack[stackTop].mode = NegRight;
					++stackTop;

					// set current node to middle child
					// gen(n-2, k-1);
					n -= 2;
					k -= 1;
					gen = true;
				}
				break;

				case NegRight: {
					changedPos = k - 1;
					changedVal = n - 2;

					// set current node to right child
					// neg(n-1, k);
					n -= 1;
					gen = false;
				}
				break;
			}
		}

		currentCombination[changedPos] = changedVal;

		double coeff = getCorrelationCoefficientGrayNext(neighbours, currentCombination, l, g, r, c, matrices, changedPos);

		if (testAndRemove(coeff, g, r, c, ccRangeMin, ccRangeMax))
			return;
	}
}

// Gray combinations generator optimized for L=2
__attribute__((hot, always_inline)) static inline
void iterativeCombGrayForL2(int* __restrict__ neighbours, const int neighboursDim, Graph* __restrict__ g, const int r,
	const int c, const double ccRangeMin, const double ccRangeMax) {
	double coeff;

	for (int n2 = neighboursDim - 1; n2 > 0; --n2) {
		double rhoRC, rhoRH1, rhoCH1;
		int h1 = neighbours[0];
		int h2 = neighbours[n2];

		rhoRC = getCorrelationCoefficient1(g->rho[r][c].v, g->rho[r][h2], g->rho[c][h2]);
		rhoRH1 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h2], g->rho[h1][h2]);
		rhoCH1 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h2], g->rho[h1][h2]);

		coeff = getCorrelationCoefficient1(rhoRC, rhoRH1, rhoCH1);
		if (testAndRemove(coeff, g, r, c, ccRangeMin, ccRangeMax))
			return;

		for (int n1 = 1; n1 < n2; ++n1) {
			h1 = neighbours[n1];

			// Value of rhoRC does not change within this loop, so it is not recalculated

			rhoRH1 = getCorrelationCoefficient1(g->rho[r][h1].v, g->rho[r][h2], g->rho[h1][h2]);
			rhoCH1 = getCorrelationCoefficient1(g->rho[c][h1].v, g->rho[c][h2], g->rho[h1][h2]);

			coeff = getCorrelationCoefficient1(rhoRC, rhoRH1, rhoCH1);
			if (testAndRemove(coeff, g, r, c, ccRangeMin, ccRangeMax))
				return;
		}
	}
}

/** Finds all the subsets adj(i)\{j} with cardinality equals to l (formally, |adj(i)\{j}| = l).

	@param Graph* g
	The reference of the Graph object representing the gene network.

	@param const int i
	Index of the selected row, that also represents the "departure" node i of an edge i->j.

	@param const int j
	Index of the selected column, that also represents the "arrival" node j of an edge i->j.

	@param const int l
	Reached dimension actually taken into account for the subset cardinality.
	In this case l is greater than 1 (formally, l > 1).

	@param double ccRangeMin

	@param double ccRangeMax

	@param int* neighbours
	Set of the indexes of the nearby nodes of the given edge i->j

	@param int* currentCombination

	@param Matrix* matrices
	Rhos matrices.

	@param GrayStackElement* grayStack

	@return nothing.
*/
__attribute__((hot, always_inline)) static inline
void findAllSubsets(Graph* __restrict__ g, const int i, const int j, const int l, const double ccRangeMin, const double ccRangeMax,
		int* __restrict__ neighbours, int* __restrict__ currentCombination, Matrix* __restrict__ matrices,
		GrayStackElement* __restrict__ grayStack) {
	int neighboursDim = 0;

	if (l == 0) {
		testAndRemove(g->rho[i][j].v, g, i, j, ccRangeMin, ccRangeMax);
	} else if (l == 1) {
		// find neighbours (when l > 0) of i without considering j
		double rho_i_j = g->rho[i][j].v;
		for (int k = 0; (k < g->nRows) && (neighboursDim < g->numNeighbours[i]); k++) {
			if (g->matrix[i][k] && (k != j)) {
				neighboursDim++;
				//double correlationCoefficient = getCorrelationCoefficient1(rho_i_j, g->rho[i][k], g->rho[j][k]);
				double correlationCoefficient = (rho_i_j - g->rho[i][k].v * g->rho[j][k].v) *
						(g->rho[i][k].vPrime * g->rho[j][k].vPrime);

				if (testAndRemove(correlationCoefficient, g, i, j, ccRangeMin, ccRangeMax))
					break;
			}
		}
	} else if (l == 2) {
		// Gray combination generator implemented in iterativeCombGray() is slower than one in iterativeComb().
		// For L=2 specialized function version is created. Use it here.

		// Note: after putting check "if (l == 2)" after this loop code was a bit slower, so keep it as-is.

		// find neighbours (when l > 0) of i without considering j
		for (int k = 0; (k < g->nRows) && (neighboursDim < g->numNeighbours[i]); k++) {
			if ((g->matrix[i][k]) && (k != j)) {
				neighbours[neighboursDim++] = k;
			}
		}

		// look for all subset of length l
		iterativeCombGrayForL2(neighbours, neighboursDim, g, i, j, ccRangeMin, ccRangeMax);
	} else {
		// find neighbours (when l > 0) of i without considering j
		for (int k = 0; (k < g->nRows) && (neighboursDim < g->numNeighbours[i]); k++) {
			if ((g->matrix[i][k]) && (k != j)) {
				neighbours[neighboursDim++] = k;
			}
		}

		// look for all subset of length l
		iterativeCombGray(neighbours, neighboursDim, l, g, i, j, ccRangeMin, ccRangeMax, currentCombination, matrices, grayStack);
	}
}

__attribute__((hot)) static
void pcAlgorithm(Graph* __restrict__ g, double inverseCNDForAlpha, int* __restrict__ neighbours, int* __restrict__ currentCombination,
		Matrix* __restrict__ matrices, GrayStackElement* __restrict__ grayStack)
{
	//Clock c;

	//c.start();
	// compute the standard deviations
	g->computeStandardDeviations();
	//c.stop("computeStandardDeviations");

	//c.start();
	// compute the correlations coefficients
	g->computeCorrelations();
	//c.stop("computeCorrelations");

	int l;
	bool hasWorked; // boolean to see that there is at least an arc i,j s.t. |adj(C, i)\{j}| >= l TO CHECK FROM THE TEXT

	// start with l = 0 & hasWorked = true
	l = -1;
	hasWorked = true;

	while ((hasWorked) && (l < g->nRows)) {
		//c.start();
		l++;
		hasWorked = false;

		matrices[l].allocate(l);

		double expVal = exp(inverseCNDForAlpha / (sqrt(g->nCols - l - 3.0) * 0.5));

		double ccRangeMin = (1.0 - expVal) / (1.0 + expVal);
		double ccRangeMax = -ccRangeMin;

		for (int i = 0; i < g->nRows; i++) {
			if (g->numNeighbours[i] <= l)
				continue;
			for (int j = 0; j < g->nRows; j++) {
				// check if exists the arc between i and j
				if (g->matrix[i][j] && (g->numNeighbours[i] > l)) {
					hasWorked = true;
					findAllSubsets(g, i, j, l, ccRangeMin, ccRangeMax, neighbours, currentCombination, matrices, grayStack);
				}
			}
		}
		//c.stop("pcAlgorithm, l", l);
	}
}

/** Body of the PC algorithm.

	@see [TODO reference to the paper]
*/
#ifdef WASM
#ifdef __cplusplus
extern "C" {
#endif
void launch() {
#else
int appMain(int argc, char* argv[]) {
#endif
	int experimentsDim; // number of experiment files to be read
	int hibridizationDim; // total number of columns of all experiment files
	int* tilesDim = NULL; // array that contains the length of each tiles read
	int tileRows; // number of rows of the tile.txt file
	intpair** tiles = NULL; // array that contains the numbers of the rows that will be extracted and represent the subgraph
	Graph* __restrict__ g = NULL; // the graph
	double alpha;
	int* __restrict__ neighbours = NULL;
	Matrix* __restrict__ matrices = NULL;
	GrayStackElement* __restrict__ grayStack = NULL;
	string* tilePath = NULL;
	string* experimentsFile = NULL;
	string* outputFile = NULL;
	string** experiments = NULL;
	string str;
	int retval;
	char buf[256];
	ostringstream strs;
	string resultsNotSaved = ""; // carries the results that were not allowed to write
	//time_t rawtime;
	//struct tm* timeinfo;
#ifndef WASM
	char* tmp;
#endif
	long long unsigned int score = 0;
	double inverseCNDForAlpha;
	int* __restrict__ currentCombination;
	int cut_results;

#ifndef WASM
	if (argc != 7) {
		cerr << "[E] wrong input parameters" << endl;
		cerr << "Usage: ./pc.exe tiles output_file alpha n_experiments n_hibridizations cutoff" << endl;
		return -1;
	}
#endif

	retval = boinc_init();

	if (retval) {
		cerr << boinc_msg_prefix(buf, sizeof(buf)) << " boinc_init() returned: " << retval;
		exit(retval);
	}



#ifdef WASM

    char** saveptr = (char**) malloc(sizeof(char**));
    char* line = strtok_r(workload, "\n", saveptr);
    char* token = strtok(line, " ");
    alpha = atof(token);
    token = strtok(NULL, " ");
    experimentsDim = atoi(token);
    token = strtok(NULL, " ");
    hibridizationDim = atoi(token);
    token = strtok(NULL, " ");
    cut_results = atoi(token);

#else
	tmp = do_gunbzip(argv[1], true);

	if (tmp == NULL) {
		exit(boinc_finish(0));
	}

	tilePath = new string(tmp); // compressed?
	outputFile = new string(argv[2]);
	experimentsDim = atoi(argv[4]);
	hibridizationDim = atoi(argv[5]);
	cut_results = atoi(argv[6]);

	if (isFloat(argv[3])) {
		alpha = atof(argv[3]);
	} else {
		alpha = 0.05;
		cerr << "[E] alpha not a float. Use the default value " << alpha << endl;
	}
#endif

	inverseCNDForAlpha = inverseComulativeNormalDistribution(1.0 - (alpha / 2.0));

	// istantiate the array of string
	experiments = new string*[experimentsDim];

	// read all the subgragraph that will be computed

#ifdef WASM
    readTile(saveptr, tilesDim, tiles, tileRows);
#else
    readTile(*tilePath, tilesDim, tiles, tileRows, experiments, experimentsDim);
#endif

	int numChkpt = readCheckpoint(*(outputFile), score);
	for (int c = numChkpt; c < tileRows; c++) {
		// update progress-bar
		progressBar(c, tileRows);

		// create the graph
		g = new Graph(tilesDim[c]);

		//Clock clk(true);
		// extract the subgraph from the complete genes file

#ifdef WASM
        readCGN(tiles[c], saveptr, experimentsDim, g, hibridizationDim);
        free(workload);
#else
        readCGN(tiles[c], experiments, experimentsDim, g, hibridizationDim);
#endif
        // print a timestamp
	    //time(&rawtime);
	    //timeinfo = localtime(&rawtime);
	    //tmp = asctime(timeinfo);
	    //cerr << "Start  @ " << tmp;

        //clk.stop("Loading");
                printf("Starting work!\n");
		Clock clk(true);
		// allocate various structures (save time)

		// allocate array for current combination
		currentCombination = new int[g->nRows];

		// alloc the neighbours array (save time)
		neighbours = new int[g->nRows];

		// allocate matrices, start with initial 30 and allocate remaining as necessary
		matrices = new Matrix[g->nRows];
		for (int n = 0; n < 30; ++n) {
			matrices[n].allocate(n);
		}

		// allocate stack for Gray combinations generator
		grayStack = new GrayStackElement[g->nRows];

		// PC-algorithm
		pcAlgorithm(g, inverseCNDForAlpha, neighbours, currentCombination, matrices, grayStack);

		/*** START CRITICAL SECTION ***/
		// tries to checkpoint
#ifndef WASM
		if (writeCheckpoint(c, score)) {
			// print output file
			fprintEdgesCSV(g, resultsNotSaved, tiles[c], *(outputFile));

			// chiude la critical section
			boinc_checkpoint_completed();
		/*** END of CRITICAL SECTION ***/

			resultsNotSaved = "";
		} else {
			// save the results in g and, hopefully, write the next time
			strs.str(std::string()); // cleans the stringstream

			for (int i = 0; i < g->nRows - 1; ++i) {
				for (int j = i + 1; j < g->nRows; ++j) {
					if (g->matrix[tiles[c][i].second][tiles[c][j].second]) {
						strs << g->probeIDs[tiles[c][i].second] << "," << g->probeIDs[tiles[c][j].second] << "\n";
					}
				}
			}

			resultsNotSaved.append(strs.str());
		}
#endif

		// save the partial score
		// originally score was incremented only every 1000 calls of testAndRemove
		score += g->score / 1000;

		// free the memory
		delete[] neighbours;

		delete[] matrices;

		delete[] grayStack;

		delete[] currentCombination;

		delete g;
        	clk.stop("Finished work!");
	}

    // print a timestamp
	//time(&rawtime);
	//timeinfo = localtime(&rawtime);
	//tmp = asctime(timeinfo);
	//cerr << "Finish @ " << tmp;
	/*** START CRITICAL SECTION ***/
	boinc_begin_critical_section();

	// write the last results not saved
#ifndef WASM
    if (strcmp(resultsNotSaved.c_str(), "") != 0) {
		BoincFile out;

		if (out.open(*(outputFile), "ab")) {
			//... if there are then save them!
			out.write(resultsNotSaved);
			out.close();
		} else {
			cerr << "[E] Cannot open \"" << *(outputFile) << "\"" << endl;
		}
	}

	//calculate the post processing to return a more compressed file
	calculatePostProcessing(*(outputFile), cut_results);
#endif

	// print score
	cerr << "|" << score << "|" << endl;

	boinc_end_critical_section();
	/*** END of CRITICAL SECTION ***/



	// free the memory
	for (int i = 0; i < tileRows; i++) {
		delete[] tiles[i];
	}
	delete[] tiles;

	for (int i = 0; i < experimentsDim; i++) {
		delete experiments[i];
	}
	delete[] experiments;

	delete[] tilesDim;
	delete experimentsFile;
	delete tilePath;
	delete outputFile;

#ifndef WASM
	return boinc_finish(0);
#endif
}

#ifdef WASM
#ifdef __cplusplus
}
#endif
#endif
