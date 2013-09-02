#include "../Include/startup_screen.h"
#include "../Include/update_screen.h"
#include "../Include/main_screen.h"
#include "../Include/menus.h"
#include "../Include/Util.h"

#include <string.h>
#include <stdlib.h>

static closure* startupScreen(closure* cl);
static closure* updateScreen(closure* cl);
static closure* setupScreen(closure* cl);
static closure* chipsLyra(closure* cl);
static closure* chipsOperadoras(closure* cl);
static closure* lyraVivo(closure* cl);
static closure* lyraTim(closure* cl);
static closure* lyraClaro(closure* cl);
static closure* lyraOi(closure* cl);
static closure* opVivo(closure* cl);
static closure* opTim(closure* cl);
static closure* opClaro(closure* cl);
static closure* opOi(closure* cl);

#ifdef _WIN32
static ret_code loginScreenInit(int* cancel) {return SUCCESS;};
static ret_code updateScreenInit() {return SUCCESS;};
static closure* mainScreen(closure* cl) {return NULL;};
#endif

enum {
	OP_TIM,
	OP_VIVO,
	OP_CLARO,
	OP_OI,
	LYRA_TIM,
	LYRA_VIVO,
	LYRA_CLARO,
	LYRA_OI
};

struct gprsConfig configs[] = {
	"189.113.164.245", "/*tim.br*/", 
	"189.113.164.245", "/*wap.vivo.com.br*/", 
	"189.113.164.245", "/*wap.claro.com.br*/", 
	"189.113.164.245", "/*wapgprs.oi.com.br*/", 
	"192.168.102.1:41157", "/*tim.br*/", 
	"192.168.102.1:41157", "/*wap.vivo.com.br*/", 
	"192.168.102.1:41157", "/*wap.claro.com.br*/", 
	"192.168.102.1:41157", "/*wapgprs.oi.com.br*/"
};

ret_code startupScreenInit()
{
	closure cl = {startupScreen, NULL, 0, NULL, NULL};
	closure* current = &cl;
	while (NULL!=current) {
		current = (*(current->func))(current);
	}
	return SUCCESS;
}

static closure* startupScreen(closure* cl)
{
	MenuItem_t items[] = {
		KEY_1, "Iniciar", NULL, updateScreen, 0,
		KEY_2, "Setup", NULL, setupScreen, 0
	};
	return showMenuWithTimeout(items,sizeof(items)/sizeof(items[0]),cl,10000,KEY_1);
}

static closure* setupScreen(closure* cl)
{
	MenuItem_t items[] = {
		KEY_1, "Chips Lyra", NULL, chipsLyra, 0,
		KEY_2, "Chips Operadoras", NULL, chipsOperadoras, 0,
		KEY_3, "Tela Anterior", NULL, startupScreen, 0,
		KEY_BACK, "", NULL, startupScreen, 1,
	};
	return showMenu(items,sizeof(items)/sizeof(items[0]),cl);
}

static closure* updateScreen(closure* cl)
{
	int cancel = 0;
	closure* prev;
	loadGPRSConfig();
	CHECK(SUCCESS==updateScreenInit());			
	CHECK(SUCCESS==loginScreenInit(&cancel));
	prev = popClosure(cl);
	return cancel? 
		prev:pushClosure(prev, mainScreen, NULL);
}

closure* chipsLyra(closure* cl)
{
	MenuItem_t items[] = {
		KEY_1, "TIM", NULL, lyraTim, 0,
		KEY_2, "VIVO", NULL, lyraVivo, 0,
		KEY_3, "CLARO", NULL, lyraClaro, 0,
		KEY_4, "OI", NULL, lyraOi, 0,
		KEY_5, "Tela Anterior", NULL, setupScreen, 0,
		KEY_BACK, "", NULL, setupScreen, 1,
	};
	return showMenu(items,sizeof(items)/sizeof(items[0]),cl);
}

closure* chipsOperadoras(closure* cl)
{
	MenuItem_t items[] = {
		KEY_1, "TIM", NULL, opTim, 0,
		KEY_2, "VIVO", NULL, opVivo, 0,
		KEY_3, "CLARO", NULL, opClaro, 0,
		KEY_4, "OI", NULL, opOi, 0,
		KEY_5, "Tela Anterior", NULL, setupScreen, 0,
		KEY_BACK, "", NULL, setupScreen, 1,
	};
	return showMenu(items,sizeof(items)/sizeof(items[0]),cl);
}

static closure* lyraVivo(closure* cl)
{
	saveGPRSConfig(&configs[LYRA_VIVO]);
	return popClosure(cl);
}

static closure* lyraTim(closure* cl)
{
	saveGPRSConfig(&configs[LYRA_TIM]);
	return popClosure(cl);
}

static closure* lyraClaro(closure* cl)
{
	saveGPRSConfig(&configs[LYRA_CLARO]);
	return popClosure(cl);
}

static closure* lyraOi(closure* cl)
{
	saveGPRSConfig(&configs[LYRA_OI]);
	return popClosure(cl);
}

static closure* opVivo(closure* cl)
{
	saveGPRSConfig(&configs[OP_VIVO]);
	return popClosure(cl);
}

static closure* opTim(closure* cl)
{
	saveGPRSConfig(&configs[OP_TIM]);
	return popClosure(cl);
}

static closure* opClaro(closure* cl)
{
	saveGPRSConfig(&configs[OP_CLARO]);
	return popClosure(cl);
}

static closure* opOi(closure* cl)
{
	saveGPRSConfig(&configs[OP_OI]);
	return popClosure(cl);
}

#ifdef _CUTEST
static void test(CuTest* tc) 
{
	struct gprsConfig* config = &configs[LYRA_VIVO];
	CuAssertTrue(tc,0==strcmp(config->hostip,"192.168.102.1:41157"));
}

CuSuite* CuGetStartupScreenSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test);
	return suite;
}
#endif











