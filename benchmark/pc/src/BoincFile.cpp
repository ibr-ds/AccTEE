/**
	Copyright (c) 2013,2016-2017, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	BoincFile.cpp
	Library for managing I/O in files within the BOINC infrastructure.

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <string>
#include <iostream>
#include <cerrno>
#include "boinc_api.h"
#include "filesys.h"
#include "BoincFile.hpp"
#include <bzlib.h>
#include <limits.h>

#define BUFLEN 32768L
#define DEBUG 0

using namespace std;

/** Opens a file within BOINC.

	@param string path
	The path of the file to (resolve and) open.

	@param string mode
	The modality in which the file will be opened.

	@return TRUE if the file is correctly opened, FALSE otherwise.
*/
bool BoincFile::open(const string& path, const string& mode) {
	bufPos = 0;
	bufSize = 0;

	if (boinc_is_standalone()) {
		wrappedFile = fopen(path.c_str(), mode.c_str()); // no need to resolve the logical path
	} else {
		string resolvedName;
		bool fail = boinc_resolve_filename_s(path.c_str(), resolvedName);

		if (fail) {
			cerr << "[E] Cannot resolve \"" << path.c_str() << "\"" << endl;
			return false;
		}

		wrappedFile = boinc_fopen(resolvedName.c_str(), mode.c_str());
	}

	return (wrappedFile != NULL);
}

/** Closes a previously opened file.

	@return TRUE il the file is correctly closed, FALSE otherwise.
*/
bool BoincFile::close() {
	return !fclose(wrappedFile);
}

/** Gets the next line of the opened file.

	@param string& out
	The read line.

	@return TRUE if it's returned a valid (non empty) string, FALSE otherwise.
*/
bool BoincFile::getLine(string& out) {
	out.clear();

	while(1) {
		if (bufPos == bufSize) {
			bufPos = 0;
			bufSize = fread(buffer, 1, BufferSize - 1, wrappedFile);
			buffer[bufSize] = '\0';
			if (0 == bufSize) // End of file or error
				break;
		}

		char* ptr = strchr(buffer + bufPos, '\n');
		if (NULL == ptr) {
			out.append(buffer + bufPos, bufSize - bufPos);
			bufPos = bufSize;
		} else {
			size_t len = ptr - buffer - bufPos;
			out.append(buffer + bufPos, len);
			bufPos += len + 1;
			break;
		}
	}

	return !out.empty();
}

/** Writes the given string into a file within BOINC.

	@param string str
	Teh string to write.

	@return TRUE if the writing is succefully done, FALSE otherwise.
*/
bool BoincFile::write(const string& str) {
	return fwrite(str.c_str(), sizeof(char), str.size(), wrappedFile) > 0;
}


char* do_gunbzip(const char* strGZ, bool bKeep) {
	unsigned char buf[BUFLEN];
	long lRead = 0;
	long lWrite = 0;
	FILE* fIn;
	char* p;
	int bzError;
	char* strOut = strdup(strGZ);
	char* strIN;
	char* strOUT;

	if((p = strrchr(strOut, '.')) == NULL) {
		return (char*) strGZ;   // no dots (ignored)
	}

	if(strcmp(p, ".bz2") != 0) {
		return (char*) strGZ;   // not .gz (ignored)
	}

	*p = '\0';

	if (boinc_is_standalone()) {
		strIN = (char*) strGZ;
	} else {
		strIN = (char*) malloc(PATH_MAX);
		bool fail = boinc_resolve_filename(strGZ, strIN, PATH_MAX);

		if (fail) {
			cerr << "[E] Cannot resolve \"" << strGZ << "\"" << endl;
			return NULL;
		}
	}

	if(!(fIn = fopen(strIN, "rb"))) {
		cerr << "gunbzip: fopen (r) error (" << strerror(errno) << ") [" << strIN << "]" << endl;
		return NULL; // error
	}

	lRead = (long) fread(buf, sizeof(char), 2, fIn);
#ifdef DEBUG
	cout << "header: " << buf[0] << " " << buf[1] << endl;
#endif
	if(buf[0] != 'B' || buf[1] != 'Z') {
#ifdef DEBUG
		cout << "gunbzip: bad header (ignored)" << endl;
#endif
		return (char*) strIN;  // not gzipped (ignored)
	}

	fseek(fIn, 0, SEEK_SET);
	BZFILE* bzf = BZ2_bzReadOpen(&bzError, fIn, 0, 0, NULL, 0);

	if (bzError != BZ_OK) {
		cerr << "gunbzip: BZ2_bzReadOpen: " << bzError << endl;
		return NULL;
	}

	if (boinc_is_standalone()) {
		strOUT = strOut;
	} else {
		strOUT = (char*) malloc(PATH_MAX);
		bool fail = boinc_resolve_filename(strOut, strOUT, PATH_MAX);

		if (fail) {
			cerr << "[E] Cannot resolve \"" << strOUT << "\"" << endl;
			return NULL;
		}
	}

	FILE* fOut = boinc_fopen(strOUT, "wb");
	if (!fOut) {
		cerr << "gunbzip: fopen (w) error (" << strerror(errno) << ")" << endl;
		return NULL; // error
	}

	while (bzError == BZ_OK) {
		lRead = BZ2_bzRead(&bzError, bzf, buf, BUFLEN);

		if (bzError == BZ_OK || bzError == BZ_STREAM_END) {
			lWrite = fwrite(buf, 1, lRead, fOut);
#ifdef DEBUG
			cout << lRead << " " << lWrite << endl;
#endif
			if (lWrite != lRead) {
				cerr << "gunbzip: short write" << endl;
				return NULL;
			}
		}
	}

	if (bzError != BZ_STREAM_END) {
		cerr << "gunbzip: bzip error after read: " << bzError << endl;
		return NULL;
	}

	BZ2_bzReadClose(&bzError, bzf);
	fclose(fIn);
	fclose(fOut);

	if (!bKeep) {
		boinc_delete_file(strGZ);
	}

	return strOUT;
}
