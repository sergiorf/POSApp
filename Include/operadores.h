#ifndef __OPERADORES_H_
#define	__OPERADORES_H_

#include "Util.h"
#include "dict.h"

extern char* g_operador;
extern char* g_operadorId;

#ifndef _WIN32
ret_code op_checkId(const char* id, const char* pwd, const char** err);
#endif

// Downloads operadores and returns a dictionary of operadores and senhas.
ret_code op_download(dict_t** operadores, dict_t** senhas);
// Content shall be NULL terminated.
ret_code op_parse(dict_t** operadores, dict_t** senhas, const char* content);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetOperadoresSuite(void);
#endif

#endif	// __OPERADORES_H_

