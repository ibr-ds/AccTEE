/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	filesys.h
	This file provides stubs for various functions from BOINC Client libs.
	They are useful during testing if you do not want to use real BOINC libs.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _FILESYS_H_
#define _FILESYS_H_

#include <string.h>
#include <string>
#include <unistd.h>

/*#ifdef __cplusplus
extern "C" {
#endif*/

extern int boinc_resolve_filename_s(const char*, std::string&);
extern int boinc_resolve_filename(const char*, char*, int len);
extern FILE* boinc_fopen(const char* path, const char* mode);
extern int boinc_delete_file(const char*);


inline int boinc_resolve_filename_s(const char* path, std::string& resolved)
{
	resolved = path;
	return false;
}

inline int boinc_resolve_filename(const char* path, char* resolved, int len)
{
	strncpy(resolved, path, len);
	return 0;
}

inline FILE* boinc_fopen(const char* path, const char* mode)
{
	return fopen(path, mode);
}

inline int boinc_delete_file(const char* path)
{
	return unlink(path);
}


/*#ifdef __cplusplus
}
#endif*/

#endif // _FILESYS_H_
