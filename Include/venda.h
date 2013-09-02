#ifndef __VENDA_H_
#define	__VENDA_H_

#include "Util.h"
#include "ingresso.h"

typedef struct {
	double quantity;
	const char* sn;
	const char* pdvId;
	const char* op;
	const char* host;
	const char* lugar;
	int qtdMax;
	int vista;
} venda_t;

typedef struct {
	char* url; 
	char* userMsg; 
	int hasError;
	int esgotado;
	long pedidoId;
	int quantity;
	char* data;
	char* hora;
	char** codigos;
	char* valorBase;
} vendaReply_t;

/* \return GUID string. Don't free */
const char* testVenda(venda_t* venda, char* msg, int length);
vendaReply_t* efetuaVenda(venda_t* venda, ingresso_t* ingresso, const char* guid);
void vendaReplyFree(vendaReply_t* reply);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetVendaSuite(void);
#endif

#endif	// __VENDA_H

