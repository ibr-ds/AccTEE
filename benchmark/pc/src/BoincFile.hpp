/**
	Copyright (c) 2013,2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	BoincFile.hpp
	Library header for managing I/O in files within the BOINC infrastructure.

	University of Trento,
	Department of Information Engineering and Computer Science

	Trento, fall 2013 - spring 2014

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#include <string>

#ifndef _BOINCFILE
#define _BOINCFILE

class BoincFile {
public:
	bool open(const std::string&, const std::string&); // Opens a file within BOINC. // args: path(string), mode(string)
	bool close(); // Closes a previously opened file.
	bool getLine(std::string&); // Gets the next line of the opened file.
	bool write(const std::string&); // Writes the given string into a file within BOINC.

private:
	static const size_t BufferSize = 50*1024;

	FILE* wrappedFile;
	char buffer[BufferSize];
	size_t bufPos;
	size_t bufSize;
};

char* do_gunbzip(const char*, bool);

#endif //_BOINCFILE
