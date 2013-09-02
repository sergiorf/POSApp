#ifndef __DATE_H_
#define	__DATE_H_

typedef struct {
	int dd;
	int mm;
	int yyyyy;
} date_t;

// Returns a null-terminated string with format dd/mm/yyyy
char* date_makeCopy(const date_t* date);
// Buffer must be of the format ddmmyyyy
date_t* date_new(const char* buf);
// Return 0 if non-valid
int date_check2(const date_t* date1,const date_t* date2);
void date_delete(date_t* date);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetDateSuite(void);
#endif

#endif	// __RELATORIO_H_

