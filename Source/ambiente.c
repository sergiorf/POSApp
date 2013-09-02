#include "../Include/ambiente.h"
#include "../Include/trex.h"
#include "../Include/http_lib.h"

ambiente_t g_Ambiente;

ret_code initAmbiente()
{
	static int started = FALSE;
	ret_code ret = started? SUCCESS:ERROR;			
	if (!started) {
#ifndef _WIN32
		const char* content;
		int length;
		CHECK(SUCCESS==db_getContent(TYPE_AMBIENTE,&content,&length));
		ret = amb_init(&g_Ambiente,content,length);
		free(content);
#else
		g_Ambiente.pdvId = (char*)calloc(strlen(TEST_PDVID)+1,1);
		strncpy(g_Ambiente.pdvId, TEST_PDVID, strlen(TEST_PDVID));
		g_Ambiente.qtdMax = 10;
		g_Ambiente.credito = 0;
		ret = SUCCESS;
#endif
	}	
	return ret;
}

ret_code amb_download(ambiente_t* ambiente)
{
	char* pdata;
	int length;
	ret_code ret = ERROR;
#define BUFSIZE 512
	char buf[BUFSIZE];
	CHECK(OK0==http_vget(buf, BUFSIZE, &pdata, &length, UPDATE_URL, g_AppConfig.szHostIP, 
				typetab[TYPE_AMBIENTE].type, g_AppConfig.szSerialNr));		
	if (NULL!=pdata) {
		if (length>0) {
			char* content = (char*)calloc(length+1,1);
			memcpy(content, pdata, length);
			CHECK(SUCCESS==(ret=amb_init(ambiente,content,length)));
			free(content);
		}
		free(pdata);		
	}
	return ret; 
}

ret_code amb_init(ambiente_t* ambiente, const char* content, int length)
{
	ret_code ret = ERROR; 
	if (NULL!=content && NULL!=ambiente) {
		const TRexChar *begin, *end, *error;
		TRex* x;		
		CHECK(NULL!=(x=trex_compile(_TREXC("QtdMax=(\\d+)&PdvId=(\\d+)" \
				"&Recibo=(\\d)&Dormir=(\\d)&Cortesias=(\\d)&Credito=(\\d)&"),&error)));
		if (trex_search(x,_TREXC(content),&begin,&end)) {
			int n = trex_getsubexpcount(x);	
			if (n==7) {
				char *qtdMax, *pdvId, *credito;					
				rx_getnext(x, 1, &qtdMax);
				rx_getnext(x, 2, &pdvId);
				rx_getnext(x, 6, &credito);
				CHECK(NULL!=qtdMax);
				CHECK(NULL!=pdvId);
				CHECK(NULL!=credito);
				ambiente->credito = atoi(credito);
				ambiente->qtdMax = atoi(qtdMax);
				ambiente->pdvId = pdvId;
				free(credito);
				free(qtdMax);					
				ret = SUCCESS;
			}
			else {
				ret = ERROR;
			}
		} /* trex_search */
		trex_free(x);
	} /* NULL!=content */
	return ret;
}

void amb_delete(ambiente_t* ambiente)
{
	if (NULL!=ambiente) {
		if (NULL!=ambiente->pdvId) {
			free(ambiente->pdvId);
		}
	}
}

#ifdef _CUTEST

void test(CuTest* tc) 
{
	static const char* content = "37f802519febf54511c725943f86df14Pdv=PDV%20TESTE%20TICPLUS&QtdMax=10&PdvId=395" \
		"&Recibo=0&Dormir=0&Cortesias=0&Credito=0&DataLogo=1312207564&EmpresaId=3"\
		"&Rotulo=DIGITAL%20INGRESSOS&Rodape=DIGITAL%20INGRESSOS%20-%20SOLU%C7%D5ES%20EFICIENTES%20EM%20GEST%C3O%20DE%20INGRESSOS"\
		"&AuditMax=0";
	int length = strlen(content);
	ambiente_t ambiente;
	CuAssertTrue(tc,SUCCESS==amb_init(&ambiente,content,length));
	CuAssertTrue(tc,10==ambiente.qtdMax);
	CuAssertTrue(tc,0==ambiente.credito);
	CuAssertTrue(tc,0==strcmp(ambiente.pdvId,"395"));
	amb_delete(&ambiente);
}

CuSuite* CuGetAmbienteSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test);
	return suite;
}

#endif

