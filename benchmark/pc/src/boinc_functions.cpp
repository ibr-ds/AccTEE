/**
	Copyright (c) 2013,2016, All Rights Reserved.
	
	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.
	
	boinc_functions.cpp
	Library relative to the exploitation of the BOINC infrastructure's
	functionalities.
	
	University of Trento,
	Department of Information Engineering and Computer Science
	
	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <string>
#include <iostream>
#include <sstream>
#include <fstream>
#include "boinc_api.h"
#include "boinc_functions.hpp"
#include "BoincFile.hpp"

using namespace std;

/** Reads the checkpoint file to know from which tile/line to restart the
	computation.
	
	@return The value of restart tile/line.
*/
int readCheckpoint(const string outputFile, long long unsigned int& score) {
	if (!boinc_is_standalone()) {
		BoincFile checkFile;
		
		if (checkFile.open(CHECKPOINT, "r")) {
			string s1, s2;

			checkFile.getLine(s1);
			checkFile.getLine(s2);

			checkFile.close();

			cerr << "Start from checkpoint: " << atoi(s1.c_str()) + 1 << endl;

			//save score
			score = atoi(s2.c_str());
			
			//start from next iteration
			return (atoi(s1.c_str()) + 1);
		}

		bool file_present = false;

		BoincFile out;
		if(out.open(outputFile, "r")){
			file_present = true;
			out.close();
		}

		//clean output file if present
		if(file_present){
			if (out.open(outputFile, "w")) {
				out.close();
			}

			cerr << "[W] Cannot open checkpoint's file, results file cleaned!" << endl;
		}
		
		return 0;
	}

	return 0;
}

/** Writes a checkpoint file.

	@param int l
	The tile/line in which the computation is arrived.

	@return TRUE if the chekpointing operation is correctly done, FALSE
	otherwise.
*/
bool writeCheckpoint(int l, long long unsigned int score) {
	if (!boinc_is_standalone()) {
		if (boinc_time_to_checkpoint()) {
			BoincFile checkFile;

			if (checkFile.open(CHECKPOINT, "wb")) {
				stringstream strs;
				//save level and score
				strs << l << "\n" << score;
				checkFile.write(strs.str());
				checkFile.close();

				return true;
			} else {
				cerr << "[W] Cannot write checkpoint's file" << endl;

				return false;
			}
		}
	}

	return false;
}

/** Advances the progress bar.
	
	@param int partial
	The actually done number of PC executions.

	@param int total
	The total number of PC execution the program will have to do.

	@return TRUE if the advancing is correctly done, FALSE otherwise.
*/
bool progressBar(int partial, int total) {
	if (!boinc_is_standalone()) {
		boinc_fraction_done((double)partial / (double)total);
		return true;
	}

	return false;
}
