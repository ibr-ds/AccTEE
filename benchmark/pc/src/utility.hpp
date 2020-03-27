/**
	Copyright (c) 2013,2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	utility.hpp
	This library will contain all the functions used by the PC-algorithm.

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _UTILITY
#define _UTILITY

#include "Graph.hpp" // Graph
#include "pc.hpp" // intpair

// Compares two pairs of the type <probe identifier, lookup index>.
bool comparator (const intpair &, const intpair &);

// Reads the file TILE modifying both sizes and data.
#ifdef WASM
void readTile(char**, int* &, intpair** &, int &);
#else
void readTile(const std::string, int* &, intpair** &, int &, std::string** &, const int);
#endif
//read the columns from the file
#ifdef WASM
void readExperiments(std::string** &, const int);
#else
void readExperiments(const std::string, std::string** &, const int);
#endif

// Reads the file CGN saving the biological data that will be used to compute the correlation coefficients.
#ifdef WASM
void readCGN(const intpair* __restrict__, char**, const int, Graph* __restrict__, const int);
#else
void readCGN(const intpair* __restrict__, std::string* __restrict__ * __restrict__, const int, Graph* __restrict__, const int);
#endif

// Computes the continous density function.
double comulativeNormalDistribution(const double);

// Computes the inverse continous density function.
double inverseComulativeNormalDistribution(const double value);

// Checks if a given string (of the form array of chars) whether representing a float number or not.
bool isFloat(const char*);

// Prints the uncutted edges of the graph in a .csv file.
void fprintEdgesCSV(Graph*, const std::string, const intpair*, const std::string);

// Counts the number of (uncutted) edges in the graph.
int countArcs(bool**, const int, const int);

// Make the post processing evaluation
void calculatePostProcessing(const std::string, const int);

#endif //_UTILITY
