#ifndef __RELATORIO_H_
#define	__RELATORIO_H_

#include "Util.h"

typedef struct {
	char* value;
	char* begin_date;
	char* end_date;
	char* report;
	// There's a length.. but the report content should be null terminated anyway..
	int report_length;
} relatorio_t;

// A new relatorio object will always be returned, even in case of error, and will need to be deleted.
// *_date will be owned by the new relatorio object.
// *_date must have the format dd/mm/yyyy
// evtId is owned by the caller.
ret_code get_relatorio(char* begin_date, char* end_date, const char* evtId, relatorio_t** relatorio);
void delete_relatorio(relatorio_t* relatorio);
relatorio_t* relatorio_new(char* begin_date, char* end_date);
bool relatorio_hasError(const relatorio_t* relatorio, char* msg, int msg_len);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetRelatorioSuite(void);
#endif

#endif	// __RELATORIO_H_

