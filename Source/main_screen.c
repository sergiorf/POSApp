#include "../Include/main_screen.h"
#include "../Include/login_screen.h"
#include "../Include/venda_screen.h"
#include "../Include/relatorio_screen.h"
#include "../Include/operadores.h"
#include "../Include/Util.h"
#include "../Include/menus.h"

#include <string.h>
#include <stdlib.h>

static closure* logoffScreen(closure* cl);
static closure* rebootScreen(closure* cl);

closure* mainScreen(closure* cl)
{
	MenuItem_t items[] = {
		KEY_1, "Venda", NULL, vendaScreen, 0,
		KEY_2, "Relatórios", NULL, relatorioScreen, 0,
		KEY_3, "Logoff", NULL, logoffScreen, 0,
		KEY_4, "Reboot", NULL, rebootScreen, 0
	};
	return showMenu(items,sizeof(items)/sizeof(items[0]), cl);
}

closure* logoffScreen(closure* cl)
{
	// 2 times back...
	closure* prev = popClosure(cl);
	return popClosure(prev);
}

closure* rebootScreen(closure* cl)
{
	SVC_RESTART ("");
	return popClosure(cl);
}

