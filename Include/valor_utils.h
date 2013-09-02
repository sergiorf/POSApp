#ifndef __VALOR_UTILS_H_
#define	__VALOR_UTILS_H_

// Prepends the currency symbol to a static buffer and returns a pointer to it
// Don't free the returned pointer
const char* valor_withCurrency(const char* value);
// Replace dot with comma (for decimals) and put dots in the thousands, returns ptr to static buffer
// e.g. 22000.00 => 22.000,00
// Don't free the returned pointer
const char* valor_format(const char* value);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetValorSuite(void);
#endif

#endif	// __VALOR_UTILS_H_

