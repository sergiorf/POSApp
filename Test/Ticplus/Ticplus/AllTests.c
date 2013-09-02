#include <stdio.h>
#include <string.h>
#include "trex.h"
#include "Util.h"
#include "dict.h"
#include "ingresso.h"
#include "http_lib.h"
#include "ambiente.h"
#include "venda.h"
#include "bmptest.h"
#include "codebar.h"
#include "modelo.h"
#include "printer.h"
#include "operadores.h"
#include "startup_screen.h"
#include "relatorio.h"
#include "date.h"
#include "valor_utils.h"
#include "str_utils.h"
#include "write_utils.h"
#include "CuTest.h"

appConfig_t g_AppConfig;
int g_conHandle = -1;

void RunAllTests(void)
{
	CuString *output = CuStringNew();
	CuSuite* suite = CuSuiteNew();
	char* op = "1125";
	CHECK(SUCCESS==util_init(&g_AppConfig));
	CHECK(SUCCESS==initAmbiente());
	g_operadorId = strdup(op);
	/*
	CuSuiteAddSuite(suite, CuGetTRexSuite());
	CuSuiteAddSuite(suite, CuGetUtilSuite());
	CuSuiteAddSuite(suite, CuGetDictSuite());
	CuSuiteAddSuite(suite, CuGetIngressoSuite());
	CuSuiteAddSuite(suite, CuGetHttpSuite());
	CuSuiteAddSuite(suite, CuGetAmbienteSuite());
	CuSuiteAddSuite(suite, CuGetVendaSuite());
	*/
	CuSuiteAddSuite(suite, CuGetBmpSuite());
	/*
	CuSuiteAddSuite(suite, CuGetCodebarSuite());
	CuSuiteAddSuite(suite, CuGetModeloSuite());	
	CuSuiteAddSuite(suite, CuGetPrinterSuite());	
	CuSuiteAddSuite(suite, CuGetOperadoresSuite());	
	CuSuiteAddSuite(suite, CuGetStartupScreenSuite());		
	CuSuiteAddSuite(suite, CuGetRelatorioSuite());			
	CuSuiteAddSuite(suite, CuGetDateSuite());	
	CuSuiteAddSuite(suite, CuGetValorSuite());	
	CuSuiteAddSuite(suite, CuGetStrUtilsSuite());
	CuSuiteAddSuite(suite, CuGetWriteUtilsSuite());
	*/
	CuSuiteRun(suite);
	CuSuiteSummary(suite, output);
	CuSuiteDetails(suite, output);
	printf("%s\n", output->buffer);
	for(;;);
}

int main(void)
{
	RunAllTests();
}
