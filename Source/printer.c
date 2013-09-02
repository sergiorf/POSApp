#include "../Include/printer.h"
#include "../Include/modelo.h"
#include "../Include/ambiente.h"
#include "../Include/operadores.h"
#include "../Include/codebar.h"
#include "../Include/Util.h"
#include "../Include/str_utils.h"
#include "bmputils.h"
#include "bmpfile.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32
#define DEV_COM4 0
static int open(int dev,int a);
static void close(int dev);
static int p3700_init(short handle, short a);
static int p3700_print(short handle, unsigned char* text);
static int p3700_close(short handle);
static void p3700_select_font(short handle, short a, short b);
static int print_image(int offset, const char* filename);
#else
#include <printer.h>
#endif

static struct keys {
	const char* orig;
	const char* newval;
} keystab[] = {
	"$(Codigo)","",
	"$(PedidoId)","",
	"$(Lote)","",
	"$(Pdv)","",
	"$(Data)","",
	"$(Hora)","",
	"$(Tipo)","",
	"$(Valor)","",
	"$(ValorBase)","",
	"$(TaxaAdm)","",
	"$(Operador)","",
	"<br/>","\n",
	"<br>","\n",
	"&nbsp;"," ",
	"<b>","",
	"</b>","",
	"R$$","R$",
};

enum {
	CODIGO,
	PEDIDOID,
	LOTE,
	PDV,
	DATA,
	HORA,
	TIPO,
	VALOR,
	VALORBASE,
	TAXAADM,
	OPERADOR
};

#define BARCODE_FILE "barcode.bmp"

static void fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
      fgetc(fp);
}

static ret_code print_singleVenda(modelo_t* modelo, struct keys* keys, int* handle);
static void delete_keys(struct keys* keys);
static void init_keys(venda_t* venda, ingresso_t* ingresso, 
		vendaReply_t* reply, struct keys* keys);
static ret_code print_print(int* handle, const char* content,
	report_t* report, reporttype_t type);
static void print_open(int* handle);
static void print_close(int* handle);

#ifdef _WIN32
void print_open(int* handle){}
void print_close(int* handle){}
#else
void print_open(int* handle)
{
	open_block_t parm;
	CHECK(ERROR!=(*handle = open(DEV_COM4,0)));	

	//Set the Comm Parameters
	memset(&parm,0,sizeof(parm));
	parm.rate = Rt_19200;
	// ITP is always set to 19200 baud
	parm.format = Fmt_A8N1 | Fmt_auto |Fmt_RTS;
	// ITP is always set at 8N1
	parm.protocol = P_char_mode;
	parm.parameter = 0;
	set_opn_blk(*handle, &parm);
	SVC_WAIT(200);

	CHECK(0==p3700_init(*handle,6));		

	SVC_WAIT(100);
}

void print_close(int* handle)
{
	p3700_close(*handle);
	close(*handle);			
}
#endif

static void print_centered(int handle, int num_cols, unsigned char* text)
{
	char* centered = str_multilineCenter((const char*)text, num_cols);
	p3700_print(handle,(unsigned char*)centered);				
	free(centered);
}

void print_text(int handle, report_t* report, unsigned char* text) 
{
	short fontsize;
	int num_cols;
	if (2!=report->fontsize) {
		fontsize = 0x00;
		num_cols = 42;
	} else {
		fontsize = 0x02;
		num_cols = 24;
	}
	p3700_select_font(handle, fontsize, 0);
	if (CENTER == report->alignment) {
		print_centered(handle,num_cols,text);
	} else {
		p3700_print(handle,text);				
	}	
}

ret_code print_print(int* handle, const char* content, 
		report_t* report, reporttype_t type)
{	
	char* old, *newstr;
	int i;
	int num = sizeof(keystab)/sizeof(keystab[0]);
	ret_code ret = SUCCESS;
	old = strdup(content);
	for (i=0;i<num;i++) {
		newstr = strReplace(old,keystab[i].orig,keystab[i].newval);
		if (NULL!=newstr) {
			free(old);
			old = newstr;
		}
	}
	// I have to reopen the device for every print, otherwise the 
	// image is not printed correctly.
	print_open(handle);
	if (TEXT==type) {		
		print_text(*handle,report,(unsigned char*)old);
	} else if (BARCODE==type) {
		int offset = 5;
		CHECK(0==ZBarcode_gentofile(keystab[CODIGO].newval,BARCODE_FILE));
		print_image(offset,BARCODE_FILE);	
	} else if (IMAGE==type) {
		const char* file = LOGO_MONO_FILENAME;
		uint16_t width, height;		
#define PX_WIDTH 176
		// Printer supports monochrome bitmaps only
		if (BMP_OK==bmp_readSize(file, &width, &height)) {
			if (width>0 && width<PX_WIDTH) {		
				int offset = 0;
				if (CENTER==report->alignment) {
					offset = (PX_WIDTH-width)/2;
				}
				print_image(offset,LOGO_MONO_FILENAME);
			}
		} 
	}
	print_close(handle);
	if (NULL!=old) free(old);
	return ret;
}

ret_code print_singleVenda(modelo_t* modelo, struct keys* keys, int* handle)
{
	report_t* report;
	int i;
	ret_code ret = ERROR;
	for (i=0;i<modelo->num_reports;i++) {
		report = modelo->reports[i];
		switch(report->report_type) {
			case TEXT:
				ret = print_print(handle,report->content,report,TEXT);
				break;
			case BARCODE:
				if (I2OF5==report->barcode_type) { 
					ret = print_print(handle,report->barcode_value,report,BARCODE);
					if (SUCCESS==ret)
						ret = print_print(handle,report->content,report,TEXT);
				}
				break;
			case IMAGE:
				ret = print_print(handle,report->content,report,IMAGE);
				break;
		}			
		if (SUCCESS!=ret)
			break;
	}		
	return ret;
}

ret_code print_venda(venda_t* venda,ingresso_t* ingresso,vendaReply_t* reply)
{
	int print_handle;
	modelo_t* modelo;
	ret_code ret;
	if (SUCCESS==(ret=modelo_download2(ingresso,&modelo))) {
		int i;
		CHECK(NULL!=reply);
		init_keys(venda,ingresso,reply,(struct keys*)&keystab);
		for (i=0;i<reply->quantity&&SUCCESS==ret;i++) {
			keystab[CODIGO].newval = reply->codigos[i];
			ret = print_singleVenda(modelo,(struct keys*)&keystab,&print_handle);
		}		
		delete_keys((struct keys*)&keystab);
		modelo_delete(modelo);		
	}	
	return ret;
}

ret_code print_relatorio(relatorio_t* relatorio)
{
	int handle, len, i;
	ret_code ret = SUCCESS;
	char* newStr, *old = strdup(relatorio->report);
	int num = sizeof(keystab)/sizeof(keystab[0]);
	for (i=0;i<num;i++) {
		newStr = strReplace(old,keystab[i].orig,keystab[i].newval);
		if (NULL!=newStr) {
			free(old);
			old = newStr;
		}
	}
	// Incrementar 5 '\n' ao imprimir o relatório	
	len = strlen(old);
	newStr = (char*)realloc(old,len+10);
	if (NULL!=newStr) {
		memcpy(newStr+len,"\n\n\n\n\n\n\n\n\n\0",10);
		old = newStr;
		// I have to reopen the device for every print, otherwise the 
		// image is not printed correctly.
		print_open(&handle);
		p3700_select_font(handle, 0x00, 0);
		p3700_print(handle,(unsigned char*)old);		
		print_close(&handle);
	}
	free(old);
	return ret;
}

void init_keys(venda_t* venda, ingresso_t* ingresso, 
		vendaReply_t* reply, struct keys* keys)
{
#define BUFLEN 128
	char* pedidoId = (char*)calloc(BUFLEN,1);
	char* valor = (char*)calloc(BUFLEN,1);
	sprintf(pedidoId,"%d",reply->pedidoId);
	keys[CODIGO].newval = "";	
	keys[PEDIDOID].newval = pedidoId;	
	keys[LOTE].newval = ingresso->lote;
	keys[PDV].newval = venda->pdvId;
	keys[DATA].newval = reply->data;
	keys[HORA].newval = reply->hora;
	keys[TIPO].newval = str_toupper(ing_getTipoDesc(ingresso));
	sprintf(valor,"%.02f\0",atof(reply->valorBase)+atof(ingresso->taxaAdm));
	keys[VALOR].newval = valor;
	keys[VALORBASE].newval = reply->valorBase;
	keys[TAXAADM].newval = ingresso->taxaAdm;
	keys[OPERADOR].newval = venda->op;
}

void delete_keys(struct keys* keys)
{
	if (NULL!=keys) {
		if (NULL!=keys[PEDIDOID].newval) {
			free((void*)keys[PEDIDOID].newval);
		}
		if (NULL!=keys[VALOR].newval) {
			free((void*)keys[VALOR].newval);
		}
		if (NULL!=keys[TIPO].newval) {
			free((void*)keys[TIPO].newval);
		}
	}
}

#ifdef _CUTEST
static void fillVendaInfo(CuTest* tc, const char* op_id, ambiente_t* ambiente, venda_t* venda)
{
	venda->quantity = 2;
	venda->sn = g_AppConfig.szSerialNr;
	venda->pdvId = ambiente->pdvId;
	venda->op = op_id;
	venda->host = g_AppConfig.szHostIP;
	venda->lugar = "0";
	venda->qtdMax = ambiente->qtdMax;
}

void test_print(CuTest* tc)
{
	ambiente_t ambiente;
	dict_t* operadores, *senhas, *ingressos;
	dictent_t ent;
	dictit_t it;
	venda_t venda;

	amb_download(&ambiente);
	op_download(&operadores,&senhas);
	ent = dict_getfirst(operadores);
	CuAssertTrue(tc,NULL!=ent);
	CuAssertTrue(tc,NULL!=ent->name);
	fillVendaInfo(tc,ent->name,&ambiente,&venda);

	CuAssertTrue(tc,SUCCESS==ing_download(&ingressos));
	CuAssertTrue(tc,NULL!=ingressos);
	CuAssertTrue(tc,0<ingressos->size);
	it = dict_it_start();
	while (NULL!=(ent=dict_it_next(ingressos,it))) {
#define BUFSIZE 128
		char buf[BUFSIZE];
		const char* guid;
		vendaReply_t* reply;
		ingresso_t* ingresso = (ingresso_t*)ent->obj;
		CuAssertTrue(tc,NULL!=(guid=testVenda(&venda,buf,BUFSIZE)));
		CuAssertTrue(tc,NULL!=(reply=efetuaVenda(&venda,ingresso,guid)));
		if (!reply->hasError) {
			CuAssertTrue(tc,SUCCESS==print_venda(&venda,ingresso,reply));
		}
		vendaReplyFree(reply);
	}
	dict_it_end(it);
	dict_free(ingressos);
	dict_free(senhas);
	dict_free(operadores);
	amb_delete(&ambiente);
}

void test_printRelatorio(CuTest* tc)
{
	const char* begin = "01/02/2013";
	const char* end   = "16/05/2013";		
	relatorio_t* relatorio;
	CuAssertTrue(tc,SUCCESS==get_relatorio(strdup(begin),strdup(end),"2",&relatorio));
	CuAssertTrue(tc,SUCCESS==print_relatorio(relatorio));
	delete_relatorio(relatorio);
}

CuSuite* CuGetPrinterSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_print);
	SUITE_ADD_TEST(suite, test_printRelatorio);
	return suite;
}
#endif

#ifdef _WIN32
#define DEV_COM4 0
static int open(int dev,int a) {return SUCCESS;}
static void close(int dev) {return;}
static int p3700_init(short handle,short a) {return 0;}
static int p3700_print(short handle,unsigned char* text) {return 1;}
static int p3700_close(short handle) {return 0;}
static void p3700_select_font(short handle, short a, short b) {}
static int print_image(int offset, const char* filename) {return 0;}
#endif

