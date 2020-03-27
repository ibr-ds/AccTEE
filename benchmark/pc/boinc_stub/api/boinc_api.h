/**
	Copyright (c) 2016, All Rights Reserved.

	This software is in the public domain, furnished "as is", without technical
	support, and with no warranty, express or implied, as to its usefulness for
	any purpose.

	boinc_api.h
	This file provides stubs for various functions from BOINC Client libs.
	They are useful during testing if you do not want to use real BOINC libs.

	University of Trento,
	Department of Information Engineering and Computer Science

	Authors: (alphabetically ordered) Francesco Asnicar, Luca Masera,
			 Paolo Morettin, Nadir Sella, Thomas Tolio.

	Optimizations by Daniel Fruzynski
*/

#ifndef _BOINC_API_H_
#define _BOINC_API_H_

/*#ifdef __cplusplus
extern "C" {
#endif*/

extern int boinc_is_standalone(void);
extern int boinc_time_to_checkpoint();
extern int boinc_fraction_done(double);
extern int boinc_init(void);
extern int boinc_finish(int status);
extern char* boinc_msg_prefix(char*, int);
extern int boinc_checkpoint_completed(void);
extern void boinc_begin_critical_section();
extern void boinc_end_critical_section();

static int g_standalone = getenv("BOINC_SA") ? atoi(getenv("BOINC_SA")) : 0;

inline int boinc_is_standalone(void)
{
	return g_standalone;
}

inline int boinc_time_to_checkpoint()
{
	return 0;
}

inline int boinc_fraction_done(double)
{
	return 0;
}

inline int boinc_init(void)
{
	return 0;
}

inline int boinc_finish(int /*status*/)
{
	return 0;
}

inline char* boinc_msg_prefix(char*, int)
{
	return (char*)"";
}

inline int boinc_checkpoint_completed(void)
{
	return 0;
}

inline void boinc_begin_critical_section()
{
}

inline void boinc_end_critical_section()
{
}

/*#ifdef __cplusplus
}
#endif*/

#endif // _BOINC_API_H_
