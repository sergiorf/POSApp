#include "../Include/relatorio.h"
#include "../Include/http_lib.h"
#include "../Include/ambiente.h"
#include "../Include/operadores.h"
#include "../Include/Util.h"
#include "../Include/trex.h"
#include <stdlib.h>
#include <string.h>

static ret_code relatorio_download(relatorio_t* relatorio, const char* evtId)
{		
#define BUFSIZE 512
	char buf[BUFSIZE];
	int length;
	char* pdata;
	ret_code ret = ERROR;
	CHECK(NULL!=g_operadorId);	
	ret = (OK0==http_vget(buf, BUFSIZE, &pdata, &length, RELATORIO1_URL, 
		g_AppConfig.szHostIP, relatorio->begin_date, relatorio->end_date, 
		g_AppConfig.szSerialNr, g_operadorId, g_Ambiente.pdvId,evtId))? SUCCESS:ERROR;
	if (NULL!=pdata && SUCCESS==ret) {
		pdata = (char*)realloc(pdata,length+1);
		pdata[length] = '\0';
		relatorio->value = pdata;
		ret = (OK0==http_vget(buf, BUFSIZE, &pdata, &length, RELATORIO2_URL, 
			g_AppConfig.szHostIP, relatorio->begin_date, relatorio->end_date, 
			g_AppConfig.szSerialNr, g_operadorId, g_Ambiente.pdvId,evtId))? SUCCESS:ERROR;
		if (NULL!=pdata && SUCCESS==ret) {
			pdata = (char*)realloc(pdata,length+1);
			pdata[length] = '\0';
			relatorio->report = pdata;
			relatorio->report_length = length;
		}
	} 
	return ret;
}

void delete_relatorio(relatorio_t* relatorio)
{
	if (NULL!=relatorio) {
		if (NULL!=relatorio->begin_date)
			free(relatorio->begin_date);
		if (NULL!=relatorio->end_date)
			free(relatorio->end_date);
		if (NULL!=relatorio->report)
			free(relatorio->report);
		if (NULL!=relatorio->value)
			free(relatorio->value);
	}
}

ret_code get_relatorio(char* begin_date, char* end_date, const char* evtId, relatorio_t** relatorio)
{
	*relatorio = relatorio_new(begin_date,end_date);
	return relatorio_download(*relatorio,evtId);	
}

relatorio_t* relatorio_new(char* begin_date, char* end_date)
{
	relatorio_t* relatorio = (relatorio_t*)calloc(1,sizeof(relatorio_t));		
	CHECK(NULL!=relatorio);
	relatorio->begin_date = begin_date;
	relatorio->end_date = end_date;	
	return relatorio;
}

bool relatorio_hasError(const relatorio_t* relatorio, char* msg, int msg_len)
{
	bool ret = FALSE;
	CHECK(NULL!=relatorio);
	if (NULL!=relatorio->report && NULL!=msg) {
		static char* errmsg = "O limite diário de 10\nimpressões foi atingido\0";
		TRex* x;
		const TRexChar *begin, *end, *error;
		// TREXC has some issue with accented letters
		CHECK(NULL!=(x=trex_compile(_TREXC("foi atingido"),&error)));
		if (trex_search(x,relatorio->report,&begin,&end)) {
			int n = trex_getsubexpcount(x);
			if (n>0) {
				CHECK(msg_len>0);
				strncpy(msg,errmsg,msg_len);				
				ret = TRUE;
			}
		}
	}
	return ret;
}

#ifdef _CUTEST

static void test_relatorio(CuTest* tc) 
{
	const char* begin = "01/02/2013";
	const char* end   = "16/05/2013";		
	relatorio_t* relatorio;
	CuAssertTrue(tc,SUCCESS==get_relatorio(strdup(begin),strdup(end),"2",&relatorio));
	CuAssertTrue(tc,NULL!=relatorio);
	CuAssertTrue(tc,0<relatorio->value);
	CuAssertTrue(tc,0<relatorio->report_length);
	CuAssertTrue(tc,NULL!=relatorio->report);
	CuAssertTrue(tc,!relatorio_hasError(relatorio,NULL,0));
	delete_relatorio(relatorio);
}

static void test_relatorio_hasError(CuTest* tc) 
{
	const char* content1 = "<p align=\"center\">O limite diário de 10 impressões foi atingido.</p></card>";
	const char* content2 = "<p align=\"center\"></p></card>";
	char msg[MAX_BUFF_SIZE];
	relatorio_t* relatorio = relatorio_new(NULL,NULL);
	relatorio->report = strdup(content1);
	CuAssertTrue(tc,relatorio_hasError(relatorio,msg,MAX_BUFF_SIZE));
	delete_relatorio(relatorio);
	relatorio = relatorio_new(NULL,NULL);
	relatorio->report = strdup(content2);
	CuAssertTrue(tc,!relatorio_hasError(relatorio,msg,MAX_BUFF_SIZE));
	delete_relatorio(relatorio);
}

CuSuite* CuGetRelatorioSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_relatorio);
	SUITE_ADD_TEST(suite, test_relatorio_hasError);
	return suite;
}

#endif

