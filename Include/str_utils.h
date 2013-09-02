#ifndef __STR_UTILS_H_
#define	__STR_UTILS_H_

// Returns an upper case version of the original string
// The result must be freed
char* str_toupper(const char* str);

/*
	This is a specialized version of strtok, to cope with consecutive tokens
	in the string. This function will match them separately i.o. considering 
	them a single token as the standard strok does.
*/
char* str_strtok(char* str, const char* delims);

/*
	Creates a new string with trimmed leading and ending withespaces.
	It won't trim intermediate spaces such as: " bla bla " => "bla bla"
	The returned pointer is NULL terminated and has to be freed.
*/
char* str_trim(const char* line);

/*
	Appends a substring.
	If orig is NULL, it will allocate a new buffer, otherwise it will realloc original buffer.
	The result will be NULL terminated.
*/
char* str_concat(char* orig, const char* appended);

/*
	If noncentered is smaller than screencols, it returns a new buffer of 
	size screencols and noncentered in the center.
	Otherwise, it returns a copy of noncentered.
	The returned pointer has to be freed.
*/
char* str_center(const char* noncentered, int screencols);

/*
	Centers a multiline text.
	The returned pointer must be freed.
*/
char* str_multilineCenter(const char* text, int screencols);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetStrUtilsSuite(void);
#endif

#endif	// __STR_UTILS_H_

