/**
	Copyright (c) 2013,2016-2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	Graph.hpp
	Header for the class Graph.

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <string>
#include <time.h>

#ifndef _GRAPH
#define _GRAPH

struct RHO
{
	double v; // represents values of the correlation matrix
	double vPrime; // precalculated values: rho' = 1/sqrt(1-rho^2)
};

/** This class collects all the information to represent the graph.
*/
class Graph {
public:
	Graph(const int); // constructor : initializes the matrix and the numNeighbours
	~Graph(void); // destructor : frees the allocated memory

	bool* __restrict__ * __restrict__ matrix; // matrix that represents the graph
	int nRows; // represents the number of rows of matrix (and columns), bioData, means, standardDeviations and numNeighbours
	int nRowsAligned; // number of rows aligned to vector boundary
	int nRowsFullVects; // number of rows which fully fits vectors
	int nCols; // represents the number of columns of bioData
	int nColsAligned; // number of columns aligned to vector boundary
	int nColsFullVects; // number of columns which fully fits vectors
	double* __restrict__ * __restrict__ bioData; // matrix that will contains the data to compute Pearson coefficient for the d-separation test
	double* __restrict__ means; // array that contains the means for each node in the graph
	std::string* __restrict__ probeIDs; // array that contains the name of each probe taken in account
	double* __restrict__ standardDeviations; // array that contains the standard deviations for each node in the graph
	int* __restrict__ numNeighbours; // represents the number of adjacents for each node (thought as column vector)
	RHO* __restrict__ * __restrict__ rho; // represents the correlation matrix
	long long unsigned int score;

	void initBioDataMatrix(const int); // initialize bioData matrix
	void computeStandardDeviations(void); // computes the standard deviation for each node in the graph
	void computeCorrelations(void); // computes the correlation coefficient of the base case, and store it in rho
	void shuffleInputProbes(const time_t); // Shuffles the probes passed ad input.

private:
	void initializeMatrix(bool* __restrict__ * __restrict__, const int, const int); // initializes the boolean matrix to 'true', but the diagonal, setted to 'false'
	void initializeNeighbours(int* __restrict__, const int, const int); // initializes the array numNeighbours with the value dim-1, since the initial graph is connected
	void initializeZero(double*, const int, const int); // initializes the given array till dim to 0.0
};
#endif //_GRAPH
