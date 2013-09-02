#include "../Include/ingresso.h"
#include "../Include/trex.h"
#include "../Include/http_lib.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAXBUF 100

ret_code createDictFromData(dict_t** ingressos, char* data)
{	
	ret_code ret = ERROR;
	if (NULL!=data) {
		char* newcontent, *tmp2;
		const char* tmp;
		const TRexChar *begin, *end, *error;
		TRex* x;	
		int index = 0;
		*ingressos = dict_create();
		CHECK(NULL!=*ingressos);
		(*ingressos)->cleanup = cleanupIngresso;		
		// work-around for a regexp bug
		tmp2 = strReplace(data,"ChaveImg=&","ChaveImg=0&");			
		CHECK(NULL!=tmp2);
		newcontent = strReplace(tmp2,"ValorP=&","ValorP=0.00&");
		if (NULL==newcontent) {
			newcontent = tmp2;
		} else {
			free(tmp2);
		}
		tmp = newcontent;		
		CHECK(NULL!=(x=trex_compile(_TREXC("EventoId=(\\d+)&Id=(\\d+)&LoteId=(\\d+)&Ingresso=([\\c\\w\\s\\p]+)&" \
				"Lote=([\\c\\w\\s\\p]+)&Sexo=(\\w)&Tipo=(\\w)&Limitado=(\\d)&ValorV=([\\d\\p]+)&ValorP=([\\d\\p]+)&"\
				"Esgotado=(\\d)&Lugar=(\\d+)&ChaveImg=([\\c\\w\\s\\p]+)&QtdImg=(\\d+)&ChaveModelo=([\\c\\w\\s\\p]+)&"\
				"TaxaAdm=([\\d\\p]+)&VendaFechada=(\\d)&Cupom=(\\d)&Vinculo=(\\w)&Multiplo=(\\d)"),&error)));
		while (trex_search(x,_TREXC(tmp),&begin,&end)) {
			int n = trex_getsubexpcount(x);			
			if (n==21) {
				char buf[MAXBUF];
				char *ingresso, *lote, *esgotado;
				ingresso_t* ing = (ingresso_t*)calloc(1,sizeof(ingresso_t));
				rx_getnext(x, 1, &ing->eventoid);
				rx_getnext(x, 2, &ing->id);
				rx_getnext(x, 3, &ing->loteid);
				rx_getnext(x, 4, &ingresso);
				rx_getnext(x, 5, &lote);
				rx_getnext(x, 6, &ing->sexo);
				rx_getnext(x, 7, &ing->tipo);
				rx_getnext(x, 8, &ing->limitado);
				rx_getnext(x, 9, &ing->valorv);
				rx_getnext(x, 10, &ing->valorp);
				rx_getnext(x, 11, &esgotado);
				rx_getnext(x, 12, &ing->lugar);
				rx_getnext(x, 13, &ing->chaveImg);
				rx_getnext(x, 14, &ing->qtdImg);
				rx_getnext(x, 15, &ing->chaveModelo);
				rx_getnext(x, 16, &ing->taxaAdm);
				rx_getnext(x, 17, &ing->vendaFechada);
				rx_getnext(x, 18, &ing->cupom);
				rx_getnext(x, 19, &ing->vinculo);
				rx_getnext(x, 20, &ing->multiplo);
				sscanf(esgotado,"%d",&ing->esgotado);
				free(esgotado);
				ing->ingresso = removeMarkup(ingresso);
				ing->lote = removeMarkup(lote);
#ifdef WIN32
				_snprintf(buf,MAXBUF,"%d",index);
#else
				snprintf(buf,MAXBUF,"%d",index);
#endif
				dict_install2(*ingressos, buf, ing->ingresso, ing);
				ret = SUCCESS;
			}
			else {
				// Corrupted DB content?
				ret = ERROR;
				break;
			}
			index++;
			tmp = end;
		} // while trex_search
		trex_free(x);
		if (NULL!=newcontent) 
			free(newcontent);
		if (NULL!=data)
			free(data);
	} // data!=NULL
	return ret;
}

void cleanupIngresso(void* ingresso)
{
	if (NULL!=ingresso) {
		ingresso_t* tmp = (ingresso_t*)ingresso;
		if (tmp->id) free(tmp->id);		
		if (tmp->loteid) free(tmp->loteid);		
		if (tmp->eventoid) free(tmp->eventoid);		
		if (tmp->ingresso) free(tmp->ingresso);
		if (tmp->lote) free(tmp->lote);
		if (tmp->sexo) free(tmp->sexo);
		if (tmp->tipo) free(tmp->tipo);
		if (tmp->limitado) free(tmp->limitado);
		if (tmp->valorp) free(tmp->valorp);
		if (tmp->valorv) free(tmp->valorv);
		if (tmp->lugar) free(tmp->lugar);
		if (tmp->chaveImg) free(tmp->chaveImg);
		if (tmp->qtdImg) free(tmp->qtdImg);
		if (tmp->chaveModelo) free(tmp->chaveModelo);
		if (tmp->taxaAdm) free(tmp->taxaAdm);
		if (tmp->vendaFechada) free(tmp->vendaFechada);
		if (tmp->cupom) free(tmp->cupom);
		if (tmp->vinculo) free(tmp->vinculo);
		if (tmp->multiplo) free(tmp->multiplo);
		free(tmp);
	}
}

static int cmpIngressos(const void** a, const void** b)
{
	int ida,idb;
	ingresso_t* inga,*ingb;
	CHECK(NULL!=a);
	CHECK(NULL!=b);
	inga = (ingresso_t*)*a;
	ingb = (ingresso_t*)*b;
	ida = atoi(inga->id);
	idb = atoi(ingb->id);
	return (ida==idb)? 0 : (ida<idb)?-1:1;
}

ingresso_t** getSortedIngressos(int* outNumIngressos, dict_t* ingressos, const char* eventoId)
{
	dict_t* tmp = dict_create();
	dictent_t ent;
	dictit_t it = dict_it_start();
	ingresso_t** result;
	CHECK(NULL!=ingressos);
	CHECK(NULL!=outNumIngressos);
	while (NULL!=(ent=dict_it_next(ingressos,it))) {
		ingresso_t* ing = ent->obj;
		int loteid;
		CHECK(NULL!=ing);
		loteid = atoi(ing->loteid);
		if (0==strcmp(ing->eventoid,eventoId) && !ing->esgotado) {
			dictent_t ent2 = dict_lookup(tmp, ing->id);
			if (NULL!=ent2) {
				ingresso_t* ing2 = ent2->obj;
				CHECK(NULL!=ing2);
				if (atoi(ing2->loteid)<=loteid) {
					continue;
				}
			}
			dict_install2(tmp,ing->id,ing->ingresso,ing);
		}
	}	
	dict_it_end(it);
	*outNumIngressos = tmp->size;
	result = (ingresso_t**)dict_toObjArray(tmp);
	qsort(result,tmp->size,sizeof(ingresso_t*),cmpIngressos);
	dict_free(tmp);
	return result;
}

bool equalIngressos(ingresso_t* a, ingresso_t* b)
{
	if (NULL==a || NULL==b)
		return FALSE;
	if (a == b)
		return TRUE;
	if (NULL==a->id || NULL==b->id || 0!=strcmp(a->id,b->id))
		return FALSE;
	if (NULL==a->loteid || NULL==b->loteid || 0!=strcmp(a->loteid,b->loteid))
		return FALSE;
	if (NULL==a->eventoid || NULL==b->eventoid || 0!=strcmp(a->eventoid,b->eventoid))
		return FALSE;
	if (NULL==a->ingresso || NULL==b->ingresso || 0!=strcmp(a->ingresso,b->ingresso))
		return FALSE;
	if (NULL==a->lote || NULL==b->lote || 0!=strcmp(a->lote,b->lote))
		return FALSE;
	if (NULL==a->sexo || NULL==b->sexo || 0!=strcmp(a->sexo,b->sexo))
		return FALSE;
	if (NULL==a->tipo || NULL==b->tipo || 0!=strcmp(a->tipo,b->tipo))
		return FALSE;
	if (NULL==a->limitado || NULL==b->limitado || 0!=strcmp(a->limitado,b->limitado))
		return FALSE;
	if (NULL==a->valorv || NULL==b->valorv || 0!=strcmp(a->valorv,b->valorv))
		return FALSE;
	if (NULL==a->valorp || NULL==b->valorp || 0!=strcmp(a->valorp,b->valorp))
		return FALSE;
	if (a->esgotado!=b->esgotado)
		return FALSE;
	if (NULL==a->lugar || NULL==b->lugar || 0!=strcmp(a->lugar,b->lugar))
		return FALSE;
	if (NULL==a->chaveImg || NULL==b->chaveImg || 0!=strcmp(a->chaveImg,b->chaveImg))
		return FALSE;
	if (NULL==a->qtdImg || NULL==b->qtdImg || 0!=strcmp(a->qtdImg,b->qtdImg))
		return FALSE;
	if (NULL==a->chaveModelo || NULL==b->chaveModelo || 0!=strcmp(a->chaveModelo,b->chaveModelo))
		return FALSE;
	if (NULL==a->taxaAdm || NULL==b->taxaAdm || 0!=strcmp(a->taxaAdm,b->taxaAdm))
		return FALSE;
	if (NULL==a->vendaFechada || NULL==b->vendaFechada || 0!=strcmp(a->vendaFechada,b->vendaFechada))
		return FALSE;
	if (NULL==a->cupom || NULL==b->cupom || 0!=strcmp(a->cupom,b->cupom))
		return FALSE;
	if (NULL==a->vinculo || NULL==b->vinculo || 0!=strcmp(a->vinculo,b->vinculo))
		return FALSE;
	if (NULL==a->multiplo || NULL==b->multiplo || 0!=strcmp(a->multiplo,b->multiplo))
		return FALSE;
	return TRUE;
}

ret_code ing_download(dict_t** ingressos)
{	
	char* pdata;
	int length;
	ret_code ret = ERROR;
#define BUFSIZE 512
	char buf[BUFSIZE];
	CHECK(OK0==http_vget(buf, BUFSIZE, &pdata, &length, UPDATE_URL, g_AppConfig.szHostIP, 
				typetab[TYPE_INGRESSOS].type, g_AppConfig.szSerialNr));		
	if (NULL!=pdata) {
		if (length>0) {
			char* content = calloc(length+1,1);
			memcpy(content, pdata, length);
			ret = createDictFromData(ingressos, content);
		}
		free(pdata);		
	}
	return ret; 
}

const char* ing_getSexDesc(const ingresso_t* ingresso)
{
	static const char* const UNISSEX = "Unissex";
	static const char* const MASCULINO = "Masculino";
	static const char* const FEMININO = "Feminino";
	CHECK(NULL!=ingresso);	
	if (0==strcmp(ingresso->sexo,"M")) {
		return MASCULINO;
	} else if (0==strcmp(ingresso->sexo,"F")) {
		return FEMININO;
	} else {
		return UNISSEX;
	}
}

const char* ing_getTipoDesc(const ingresso_t* ingresso)
{
	static const char* const INTEIRA = "Inteira";
	static const char* const MEIA = "Meia";
	if (0==strcmp(ingresso->tipo,"M")) {		
		return MEIA;
	} else  {
		return INTEIRA;
	}
}

#ifdef _CUTEST

static 	ingresso_t testData[] = {
	"1658","13523","3929","INGRESSO%20%2A%20CORTESIA%20%2A","3%BA%20LOTE","U","M","0","0.00","0.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"2","13428","2","PISTA%201","1%BA%20LOTE","F","M","0","100.00","100.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"1658","13523","3928","INGRESSO%20%2A%20CORTESIA%20%2A","2%BA%20LOTE","U","M","0","0.00","0.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"2","1","2","PISTA%205","1%BA%20LOTE","U","I","0","1.00","1.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"1658","13514","3927","INGRESSO%20FEMININO%20-%201%BA%20LOTE","1%B0%20LOTE","F","M","0","15.00","0.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"1658","13523","3927","INGRESSO%20%2A%20CORTESIA%20%2A","1%B0%20LOTE","U","M","0","0.00","0.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",
	"1658","13515","3927","INGRESSO%20MASCULINO%20-%201%BA%20LOTE","1%B0%20LOTE","M","M","0","20.00","0.00",0,"0","0","0","34bb4453","0.00","1","0","N","0",	
};

void test_getSorted(CuTest* tc)
{	
	int n, i, size = sizeof(testData)/sizeof(testData[0]);
	dict_t* ingressos = dict_create();
	ingresso_t** outdata;
	for (i=0;i<size;i++) {
		char buf[50];
		_snprintf(buf, 50, "%d", i);
		dict_install2(ingressos, buf, testData[i].ingresso, &testData[i]);
	}
	outdata = getSortedIngressos(&n, ingressos, "1658");
	CuAssertTrue(tc,3==n);
	CuAssertTrue(tc,equalIngressos(outdata[0],&testData[4]));
	CuAssertTrue(tc,equalIngressos(outdata[1],&testData[6]));
	CuAssertTrue(tc,equalIngressos(outdata[2],&testData[5]));
	dict_free(ingressos);
}

void test_getDesc(CuTest* tc)
{
	struct {
		const char* sex;
		const char* tipo;
	} hardCodedVals[] = {
		{"Unissex","Meia"},
		{"Feminino","Meia"},
		{"Unissex","Meia"},
		{"Unissex","Inteira"},
		{"Feminino","Meia"},
		{"Unissex","Meia"},
		{"Masculino","Meia"},
	};
	int i, size = sizeof(testData)/sizeof(testData[0]);
	for (i=0;i<size;i++) {
		CuAssertTrue(tc,0==strcmp(hardCodedVals[i].sex,ing_getSexDesc(&testData[i])));
		CuAssertTrue(tc,0==strcmp(hardCodedVals[i].tipo,ing_getTipoDesc(&testData[i])));
	}
}

CuSuite* CuGetIngressoSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_getSorted);
	SUITE_ADD_TEST(suite, test_getDesc);
	return suite;
}

#endif

