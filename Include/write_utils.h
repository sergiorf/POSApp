#ifndef __WRITE_UTILS_H_
#define	__WRITE_UTILS_H_

void write_multiline(const char* multine,int x,int y,int maxlines);
// Takes a multiline separated by '\n' and prints it on screen centered
void write_multilineCentered(const char* multiline);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetWriteUtilsSuite(void);
#endif

#endif	// __WRITE_UTILS_H_

