/**
	Copyright (c) 2013,2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	utlity.cpp
	File with utility functions for an efficient computation of the PC
	algorithm for gene networks expansion.

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#define M_SQRT1_2 0.70710678118654757273731092936941422522068023681640625 // definition for M_SQRT1_2
#include <cmath>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <map>
#include "utility.hpp"
#include "BoincFile.hpp"
#include "erf.h"
#include "DoubleVector.hpp"

using namespace std;

/** Compares two pairs of the type <probe identifier, lookup index>.

	@param const intpair &l
	First intpair (formally <p1,l1>).

	@param const intpair &r
	Second intpair (formally <p1,l1>).

	@return
	TRUE if p1 is less than p2 (formally p1 < p2). Otherwise FALSE.
*/
bool comparator (const intpair &l, const intpair &r) {
	return l.first < r.first;
}

/** Reads the file TILE modifying both sizes and data.

	@param const string tilePath
	Path of the file containing the selected tiles.

	@param int* &tilesDim
	Reference of the array containing the lengths of the selected tiles.

	@param intpair** &tiles
	Reference of the matrix of pairs <probe identifier, lookup index>.

	@param int &tileRows
	Reference of the number of selected tiles.

	@return nothing.
*/
#ifdef WASM
 void readTile(char** saveptr, int* &tilesDim, intpair** &tiles, int &tileRows) {
	char* line;
	tileRows = 1;

    line = strtok_r(NULL, "\n", saveptr);
    string secondLine(line);

	// read the input experiments (first line)
	//readExperiments(experiments, experimentsDim);

	//while (tile.getLine(line)) {
	//	tileRows++;
	//}

	// create tileRows arrays
	tilesDim = new int[tileRows];

	// intialize tilesDim structure
	for (int i = 0; i < tileRows; i++) {
		tilesDim[i] = 0;
	}

	tiles = new intpair*[tileRows];

	// For each line (i.e., tile), counts the number of words in order to know how many nodes there are in the subgraph.
	//for (int  i = 0; tile.getLine(line); i++) {
	//	stringstream stream(line);
    strtok(line, " ");
    tilesDim[0]++;
		while (strtok(NULL, " ") != NULL) {
			tilesDim[0]++;
		}

		// Instantiate a new array of intpair as long as the previous read.
		tiles[0] = new intpair[tilesDim[0]];
	//}

	// For each tile, extracts the index of involved probes and associates it to a (increasing) lookup index.
	for (int r = 0; r < tileRows; r++) {

        char* tmp = strtok((char*)secondLine.c_str(), " ");

        printf("%s\n", tmp);
		for (int c = 0; c < tilesDim[r]; c++) {
			int temp = atoi(tmp);
			tiles[r][c] = make_pair(temp, c);
            tmp = strtok(NULL, " ");
		}
	}

	// sort the indexes of the subgraphs
	for (int i = 0; i < tileRows; i++) {
		sort(tiles[i], tiles[i] + tilesDim[i]);
	}
 }

#else

void readTile(const string tilePath, int* &tilesDim, intpair** &tiles, int &tileRows, string** &experiments, const int experimentsDim) {
	BoincFile tile;
	string line;
	tileRows = 0;

	// count the numbers of rows
	if (!tile.open(tilePath, "r")) {
		cerr << "[E] Failed to open \"" << tilePath << "\" file" << endl;
		exit(1);
	}

	// read the input experiments (first line)
	tile.getLine(line);
	readExperiments(line, experiments, experimentsDim);

	while (tile.getLine(line)) {
		tileRows++;
	}

	tile.close();

	// create tileRows arrays
	tilesDim = new int[tileRows];

	// intialize tilesDim structure
	for (int i = 0; i < tileRows; i++) {
		tilesDim[i] = 0;
	}

	tiles = new intpair*[tileRows];

	if (!tile.open(tilePath, "r")) {
		cerr << "[E] Failed to open \"" << tilePath << "\" file" << endl;
		exit(1);
	}

	// trash the first line, it contains the experiments
	tile.getLine(line);

	// For each line (i.e., tile), counts the number of words in order to know how many nodes there are in the subgraph.
	for (int  i = 0; tile.getLine(line); i++) {
		stringstream stream(line);

		while (getline(stream, line, ' ')) {
			tilesDim[i]++;
		}

		// Instantiate a new array of intpair as long as the previous read.
		tiles[i] = new intpair[tilesDim[i]];
	}

	tile.close();

	if (!tile.open(tilePath, "r")) {
		cerr << "[E] Failed to open \"" << tilePath << "\" file" << endl;
		exit(1);
	}

	// trash the first line, it contains the experiments
	tile.getLine(line);

	// For each tile, extracts the index of involved probes and associates it to a (increasing) lookup index.
	for (int r = 0; r < tileRows; r++) {
		tile.getLine(line);
		stringstream stream(line);

		for (int c = 0; c < tilesDim[r]; c++) {
			getline(stream, line, ' ');
			int temp = atoi(line.c_str());
			tiles[r][c] = make_pair(temp, c);
		}
	}

	tile.close();

	// sort the indexes of the subgraphs
	for (int i = 0; i < tileRows; i++) {
		sort(tiles[i], tiles[i] + tilesDim[i]);
	}
 }
#endif

 /**
  *
  */
#ifdef WASM

 void readExperiments(string** &experiments, const int experimentsDim) {
    char* line;
    for(int i = 0; (line = strtok(NULL, "\n")) != NULL && (i < experimentsDim); i++) {
        experiments[i] = new string(line);
    }
 }

#else
 void readExperiments(string line1, string** &experiments, const int experimentsDim) {
	// BoincFile expp;
	string line;

	// expp.open(experimentsFile, "r");
	// expp.getLine(line);
	// expp.close();

	stringstream stream(line1);

	for (int i = 0; (getline(stream, line, ' ') && (i < experimentsDim)); i++) {
		experiments[i] = new string(line);
	}
 }
#endif

/** Reads the file CGN saving the biological data that will be used to compute the correlation coefficients.

	@param const string cgnPath
	Path of the file containing the complete gene network.

	@param const intpair* nodesPermutation
	Permutation of the nodes taken into consideration.

	@param Graph* &g
	The reference of the Graph object representing the gene network.

	@return nothing.
*/

#ifdef WASM
void readCGN(const intpair* __restrict__ nodesPermutation, char** saveptr,
	const int experimentsDim, Graph* __restrict__ g, const int hibridizationDim) {
	int column = 0;

	//line.reserve(50*1024);

	// Initializes the bioData matrix
	g->initBioDataMatrix(hibridizationDim);

	for (int expp = 0; expp < experimentsDim; expp++) {
		// counter for the subset graph (aka, tile)
		int c = 0, j = 0;

		// trash the header
        char* currLine = strtok_r(NULL, "\n", saveptr);

		// I start from 0, as the number of rows shows in the text editors
		for (int i = 0; (c < g->nRows) && (currLine = strtok_r(NULL, "\n", saveptr)) != NULL; i++) {
			// check if it is the rows chosen in readTile()
			if (i == nodesPermutation[c].first) {
                char* token = strtok(currLine, ",");

				for (j = 0; ((j - 1 + column) < hibridizationDim) && token != NULL; j++) {
                    string line(token);
					if (j == 0) { // I'm reading the first token that is the probe id
						if (expp == 0) {
							g->probeIDs[nodesPermutation[c].second] = line;
						}
					} else {
						g->bioData[nodesPermutation[c].second][j - 1 + column] = atof(line.c_str());
						g->means[nodesPermutation[c].second] += g->bioData[nodesPermutation[c].second][j - 1 + column];
					}
                    token = strtok(NULL, ",");
				}

				c++;
			}
		}

		if (j > 0) {
			column += j - 1;
		}
	}

	// compute means
	for (int i = 0; i < g->nRows; i++) {
		g->means[nodesPermutation[i].second] /= (double) g->nCols;
	}
}

#else

void readCGN(const intpair* __restrict__ nodesPermutation, string* __restrict__ * __restrict__ experiments,
	const int experimentsDim, Graph* __restrict__ g, const int hibridizationDim) {
	BoincFile cgn;
	string line;
	int column = 0;

	line.reserve(50*1024);

	// Initializes the bioData matrix
	g->initBioDataMatrix(hibridizationDim);

	for (int expp = 0; expp < experimentsDim; expp++) {
		// read the file and save the values in bioData
		if (!cgn.open(*(experiments[expp]), "r")) {
			cerr << "[E] Failed to open \"" << *(experiments[expp]) << "\" file" << endl;
			exit(1);
		}

		// counter for the subset graph (aka, tile)
		int c = 0, j = 0;

		// trash the header
		cgn.getLine(line);

		// I start from 0, as the number of rows shows in the text editors
		for (int i = 0; (c < g->nRows) && cgn.getLine(line); i++) {
			// check if it is the rows chosen in readTile()
			if (i == nodesPermutation[c].first) {
				stringstream stream(line);

				for (j = 0; ((j - 1 + column) < hibridizationDim) && getline(stream, line, ','); j++) {
					if (j == 0) { // I'm reading the first token that is the probe id
						if (expp == 0) {
							g->probeIDs[nodesPermutation[c].second] = line;
						}
					} else {
						g->bioData[nodesPermutation[c].second][j - 1 + column] = atof(line.c_str());
						g->means[nodesPermutation[c].second] += g->bioData[nodesPermutation[c].second][j - 1 + column];
					}
				}

				c++;
			}
		}

		if (j > 0) {
			column += j - 1;
		}

		cgn.close();
	}

	// compute means
	for (int i = 0; i < g->nRows; i++) {
		g->means[nodesPermutation[i].second] /= (double) g->nCols;
	}
}
#endif

/** Computes the continous density function.
	M_SQRT1_2 takes value 1/sqrt(2).

	@param const double value
	Value for which it will be computed its cumulative normal distribution.

	@return The cumulative normal distribution decimal value for the passed parameter.
*/
double comulativeNormalDistribution(const double value) {
	// return 0.5 * erfc(-value * M_SQRT1_2);
	return 0.5 * a_erfc(-value * M_SQRT1_2);
}

/** Computes the inverse continous density function.
	This function uses bisection to find returned value, so it is quite slow,
	e.g. for alpha = 0.05 it took 51 iterations to find it with expected precision.
	Not be big deal if it is called once, though.

	@param const double value
	Value for which it will be computed

	@return The inverse cumulative normal distribution decimal value for the passed parameter.
*/
double inverseComulativeNormalDistribution(const double value) {
	const int MAX_ITERATIONS = 200;
	const double TOLERANCE = 1e-20;

	int iter = 1;
	double a = 0.0, b = 10.0;
	while (iter <= MAX_ITERATIONS) {
		double c = (a + b) / 2.0;

		double cnd = comulativeNormalDistribution(c);

		if ((cnd == value) || ((b - a) / 2.0 < TOLERANCE)) {
			return c;
		}

		++iter;
		double cnd_a = comulativeNormalDistribution(a);

		if (((cnd < value) && (cnd_a < value)) || ((cnd > value) && (cnd_a > value)))
			a = c;
		else
			b = c;
	}

	cerr << "Max iteration count exceeded" << endl;
	std::exit(1);
}

/** Checks if a given string (of the form array of chars) whether representing a float number or not.

	@param const char* number
	String (or, rather, array of characters) that should represent a decimal number.

	@return TRUE if the string follows the correct format for representing a float number. FALSE otherwise.
*/
bool isFloat(const char* number) {
	bool noMorePoint = false;

	for (int i = 0; number[i] != '\0'; i++) {
		if ((number[i] < '0') || (number[i] > '9')) {
			if (number[i] == '.') {
				if (!noMorePoint) {
					noMorePoint = true;
				} else {
					return false;
				}
			}
		}
	}

	return true;
}

/** Prints the uncutted edges of the graph in a .csv file.

	@param Graph* g
	The Graph object representing the gene network.

	@param const intpair* nodesPermutation
	Permutation of the nodes taken into consideration.

	@param const string fileName
	Name of the output file.

	@return nothing.
*/
void fprintEdgesCSV(Graph* g, const string resultsNotSaved, const intpair* nodesPermutation, const string fileName) {
	BoincFile out;
	ostringstream strs;

	strs.str(std::string()); // cleans the stringstream

	if (out.open(fileName, "ab")) {
		// check if there are results not saved...
		if (strcmp(resultsNotSaved.c_str(), "") != 0) {
			//... if there are then save them!
			out.write(resultsNotSaved);
		}

		// print the edges
		for (int i = 0; i < g->nRows - 1; ++i) {
			for (int j = i + 1; j < g->nRows; ++j) {
				if (g->matrix[nodesPermutation[i].second][nodesPermutation[j].second]) {
					strs << g->probeIDs[nodesPermutation[i].second] << "," << g->probeIDs[nodesPermutation[j].second] << "\n";
				}
			}
		}

		out.write(strs.str());
		out.close();
	} else {
		cerr << "[E] Cannot open \"" << fileName << "\"" << endl;
	}
 }

/** Counts the number of (uncutted) edges in the graph.

	@param bool** matrix
	Matrix of booleans representing a tabular form of the presence and absence of all the edges in the graph.
	The boolean located in the cell of i-th row and j-th column represents the presence/absence of the edge i->j.

	@param const int rows
	The number of rows in the matrix.

	@param const int cols
	The number of rows in the matrix.

	@return The number of uncutted edges.
*/
int countArcs(bool** matrix, const int rows, const int cols) {
	int counter = 0;

	for (int i = 0; i < rows; i++) {
		for (int j = 0; j < cols; j++) {
			if (matrix[i][j]) counter++;
		}
	}

	return counter;
}

/**
 *
 */
void calculatePostProcessing(const string filename, const int cut_results) {
    map<string, int> postProcessing;
	BoincFile file;
	string line;
	ostringstream strs;

	// check outputfile format --- if it has alredy been processed
	file.open(filename, "rb");
	file.getLine(line);
	size_t found = line.find('\t');
	file.close();

	if (found != string::npos) {
		return;
	}

	file.open(filename, "rb");

	// count the numbers of rows
	while (file.getLine(line)) {
		if(postProcessing[line] == 0) {
			postProcessing[line] = 1;
		} else {
			postProcessing[line]++;
		}
	}

	file.close();

	// write the output file in the format "arc<TAB>count"
	file.open(filename, "wb");

	for (map<string, int>::iterator it = postProcessing.begin(); it != postProcessing.end(); it++) {
		if (it->second > cut_results) {
			strs << it->first << "\t" << it->second << "\n";
		}
	}

	file.write(strs.str());
	file.close();
}
