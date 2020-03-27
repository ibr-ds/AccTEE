/**
	Copyright (c) 2013,2016, All Rights Reserved.
	
	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.
	
	boinc_functions.hpp
	This library contains all the functions that will be used to interface the
	BOINC client with the BOINC server.
	
	University of Trento,
	Department of Information Engineering and Computer Science
	
	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _BOINCFUNCTIONS
#define _BOINCFUNCTIONS

#define CHECKPOINT "checkpoint.txt"

// Reads the checkpoint file to know from which tile/line to restart the computation.
int readCheckpoint(const std::string, long long unsigned int&);

// Writes a checkpoint file.
bool writeCheckpoint(int, long long unsigned int);

// Advances the progress bar.
bool progressBar(int, int);

#endif //_BOINCFUNCTIONS
