#ifndef __AMBIENTE_H
#define	__AMBIENTE_H

#include "Util.h"

typedef struct {
	const char* pdvId;
	int qtdMax;
	int credito;
} ambiente_t;

extern ambiente_t g_Ambiente;

ret_code initAmbiente();
ret_code amb_init(ambiente_t* ambiente, const char* content, int length);
ret_code amb_download(ambiente_t* ambiente);
void amb_delete(ambiente_t* ambiente);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetAmbienteSuite(void);
#endif

#endif	// __AMBIENTE_H

