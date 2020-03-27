/**
	Copyright (c) 2013,2016-2017, All Rights Reserved.
	
	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.
	
	Graph.cpp
	File defining the class Graph, representing the graph/network in which we
	will run the PC algorithm for causality discovering
	
	University of Trento,
	Department of Information Engineering and Computer Science
	
	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <cmath>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include "Graph.hpp"
#include "DoubleVector.hpp"

/**	Constructor method.
	
	The graph is implemented as a matrix of edges. Each cell has coordinates
	(x,y), representing the edge x->y. If the edge exists within the graph, the
	correspondent cell contains the value TRUE, otherwise FALSE.
	
	@param const int dim
	The dimension with which the matrix will be built.
*/
Graph::Graph(const int dim) {
	nRows = dim;
	nRowsAligned = DoubleVector::align_to_element_count(dim);
	nRowsFullVects = nRowsAligned > nRows ? nRowsAligned - DoubleVector::ElementCount : nRowsAligned;

	numNeighbours = new int[nRows];

	// create the bool matrix
	matrix = new bool*[nRows];

	for (int i = 0; i < nRows; i++) {
		matrix[i] = new bool[nRows];
	}

	means = DoubleVector::alloc(nRowsAligned);
	probeIDs = new std::string[nRows];
	standardDeviations = DoubleVector::alloc(nRowsAligned);

	// create rho matrix
	rho = new RHO*[nRows];

	for (int i = 0; i < nRows; i++) {
		rho[i] = (RHO*)DoubleVector::alloc(nRowsAligned * 2);
	}

	score = 0;

	// initialize matrix, numNeighbours and l
	initializeMatrix(matrix, nRows, nRowsAligned);
	initializeNeighbours(numNeighbours, nRows, nRowsAligned);
	initializeZero(means, nRows, nRowsAligned);
	initializeZero(standardDeviations, nRows, nRowsAligned);
}

/** Destructor method.
	
	The method simply deallocates all the previously allocated data.
*/
Graph::~Graph(void) {
	// empty the memory for matrix
	for (int i = 0; i < nRows; i++) {
		delete[] matrix[i];
	}
	delete[] matrix;

	// empty the memory for bioData
	for (int i = 0; i < nRows; i++) {
		DoubleVector::free(bioData[i]);
	}
	delete[] bioData;

	// empty the memory for means
	DoubleVector::free(means);

	// empty the memory for probeIDs
	delete[] probeIDs;

	// empty the memory for standardDeviations
	DoubleVector::free(standardDeviations);

	// empty the memory for numNeighbours
	delete[] numNeighbours;

	// empty the memory for rho
	for (int i = 0; i < nRows; i++) {
		DoubleVector::free((double*)rho[i]);
	}
	delete[] rho;
}

void Graph::initBioDataMatrix(const int columns) // initialize bioData matrix
{
	nCols = columns;
	nColsAligned = DoubleVector::align_to_element_count(columns);
	nColsFullVects = nColsAligned > nCols ? nColsAligned - DoubleVector::ElementCount : nColsAligned;
	
	bioData = new double*[nRows];

	for (int i = 0; i < nRows; i++) {
		bioData[i] = DoubleVector::alloc(nColsAligned);
		
		if (nColsAligned > nCols) {
			std::memset(bioData[i] + nCols, 0, (nColsAligned - nCols) * sizeof(double));
		}
	}
}

/** Shuffles the given probes.

	@param const time_t seed
	Seed for shuffling univocally the probes passed as input.

	@return nothing.
*/
void Graph::shuffleInputProbes(const time_t seed) {

	std::srand(seed);
	std::random_shuffle(&bioData[0], &bioData[nRows]);

	std::srand(seed);
	std::random_shuffle(&means[0], &means[nRows]);

	std::srand(seed);
	std::random_shuffle(&probeIDs[0], &probeIDs[nRows]);
}

/** Compute the standard deviations for each node in the graph.

	@return nothing.
*/
// TODO SIMD; check performance!
void Graph::computeStandardDeviations(void) {
	/*for (int r = 0; r < nRows; r++) {
		double mean_r = means[r];
		for (int c = 0; c < nCols; c++) {
			standardDeviations[r] += pow((bioData[r][c] - mean_r), 2);
		}

		standardDeviations[r] /= (double) nCols;
		standardDeviations[r] = sqrt(standardDeviations[r]);
	}*/
	
	for (int r = 0; r < nRows; r++) {
		const double mean_r = means[r];
		const DoubleVector v_mean_r(mean_r);
		DoubleVector v_sum;
		int c;
		for (c = 0; c < nColsFullVects; c += DoubleVector::ElementCount) {
			//standardDeviations[r] += pow((bioData[r][c] - mean_r), 2);
			DoubleVector v_tmp = DoubleVector(&bioData[r][c]) - v_mean_r;
			v_sum.mul_add_self(v_tmp, v_tmp);
		}
		standardDeviations[r] = v_sum.sum_elements();
		for (; c < nCols; ++c) {
			standardDeviations[r] += pow((bioData[r][c] - mean_r), 2);
		}
	}

	const DoubleVector v_nCols(static_cast<double>(nCols));
	for (int r = 0; r < nRows; r += DoubleVector::ElementCount) {
		//standardDeviations[r] /= (double) nCols;
		//standardDeviations[r] = sqrt(standardDeviations[r]);
		DoubleVector v(&standardDeviations[r]);
		v /= v_nCols;
		v.sqrt_self();
		v.store(&standardDeviations[r]);
	}
}

/** Compute the correlation coefficient of the base case, and store it in rho.

	@return nothing.
*/
// TODO SIMD; check performance!
void Graph::computeCorrelations(void) {
	/*double covariance = 0.0;

	for (int i = 0; i < nRows; i++) {
		double mean_i = means[i];
		double standardDeviation_i = standardDeviations[i];
		for (int j = 0; j < nRows; j++) {
			covariance = 0.0;
			double mean_j = means[j];

			for (int k = 0; k < nCols; k++) {
				covariance += (bioData[i][k] - mean_i) * (bioData[j][k] - mean_j);
			}

			// divide covariance by nCols
			covariance /= nCols;

			// covariance(i, j) / sigma(i) * sigma(j)
			rho[i][j] = covariance / (standardDeviation_i * standardDeviations[j]);
		}
	}*/
	
	const DoubleVector v_nCols(static_cast<double>(nCols));
	const DoubleVector v_1_0(1.0);
	
	for (int i = 0; i < nRows; i++) {
		const double mean_i = means[i];
		const DoubleVector v_mean_i(mean_i);

		double standardDeviation_i = standardDeviations[i];

		for (int j = 0; j <= i; j++) {
			const double mean_j = means[j];
			const DoubleVector v_mean_j(mean_j);
			DoubleVector v_sum;

			int k;
			for (k = 0; k < nColsFullVects; k += DoubleVector::ElementCount) {
				//covariance += (bioData[i][k] - mean_i) * (bioData[j][k] - mean_j);
				v_sum.mul_add_self(DoubleVector(&bioData[i][k]) - v_mean_i,
								   DoubleVector(&bioData[j][k]) - v_mean_j);
			}
			double covariance = v_sum.sum_elements();
			for (; k < nCols; ++k) {
				covariance += (bioData[i][k] - mean_i) * (bioData[j][k] - mean_j);
			}
			
			covariance /= nCols;

			// covariance(i, j) / sigma(i) * sigma(j)
			rho[j][i].v = rho[i][j].v = covariance / (standardDeviation_i * standardDeviations[j]);
			rho[j][i].vPrime = rho[i][j].vPrime = 1.0 / sqrt(1.0 - rho[i][j].v * rho[i][j].v);
		}
	}
}

/** Initialize the boolean matrix to TRUE, but the diagonal, set to FALSE.
	
	@param bool** matrix
	Matrix to initialize.

	@param const int dim
	Dimension (number of rows, and then also number of columns) of the matrix.

	@return nothing.
*/
void Graph::initializeMatrix(bool* __restrict__ * __restrict__ matrix, const int dim, const int /*dimAligned*/) {
	for (int i = 0; i < dim; i++) {
		std::memset(matrix[i], true, dim * sizeof(bool));
		matrix[i][i] = false;
	}
}

/** Initialize the array numNeighbours with the value dim - 1, since the
	initial graph is connected.
	
	@param int* numNeighbours
	List of the number of neighbours for each node of the graph.
	
	@param const int dim
	The dimension of the list of neighbours.
	
	@return nothing.
*/
void Graph::initializeNeighbours(int* __restrict__ numNeighbours, const int dim, const int dimAligned) {
	for (int i = 0; i < dim; i++) {
		numNeighbours[i] = dim - 1;
	}
	if (dimAligned > dim) {
		std::memset(numNeighbours + dim, 0, (dimAligned - dim) * sizeof(int));
	}
}

/** Initialize the given array till dimension to 0.0.

	@param double* array
	[TODO]

	@param const int dim
	The dimension of the array of probes.

	@return nothing.
*/
void Graph::initializeZero(double* __restrict__ array, const int /*dim*/, const int dimAligned) {
	std::memset(array, 0, dimAligned * sizeof(double));
}
