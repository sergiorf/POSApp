#ifndef __PRINTER_H_
#define	__PRINTER_H_

#include "venda.h"
#include "ingresso.h"
#include "relatorio.h"

ret_code print_venda(venda_t* venda,ingresso_t* ingresso,vendaReply_t* reply);
ret_code print_relatorio(relatorio_t* relatorio);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetPrinterSuite(void);
#endif

#endif	// __PRINTER_H_

