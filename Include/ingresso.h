#ifndef __INGRESSO_H_
#define	__INGRESSO_H_

#include "dict.h"

typedef struct {
	char* eventoid;
	char* id;
	char* loteid;
	char* ingresso;
	char* lote;
	char* sexo;
	char* tipo;
	char* limitado;
	char* valorv;
	char* valorp;
	int esgotado;
	char* lugar;
	char* chaveImg;
	char* qtdImg;
	char* chaveModelo;
	char* taxaAdm;
	char* vendaFechada;
	char* cupom;
	char* vinculo;
	char* multiplo;	
} ingresso_t;

void cleanupIngresso(void* ingresso);
bool equalIngressos(ingresso_t* a, ingresso_t* b);
// Returned array is to be freed, but not the pointers contained within
ingresso_t** getSortedIngressos(int* outNumIngressos, dict_t* ingressos, const char* eventoId);
// Data is NULL terminated and will be freed
ret_code createDictFromData(dict_t** ingressos, char* data);
// Downloads ingresso informations and returns a dictionary of ingressos.
ret_code ing_download(dict_t** ingressos);
const char* ing_getSexDesc(const ingresso_t* ingresso);
const char* ing_getTipoDesc(const ingresso_t* ingresso);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetIngressoSuite(void);
#endif

#endif	// __INGRESSO_H_


