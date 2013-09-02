#include "../Include/venda.h"
#include "../Include/http_lib.h"
#include "../Include/Util.h"
#include <string.h>
#include <stdlib.h>

#define BUFSIZE 512
#define INPUTSIZE 128

static char vendaGUID[BUFSIZE];
static vendaReply_t* vendaReplyNew();
static vendaReply_t* processVendaReply(const char* data, int length);

const char* testVenda(venda_t* venda, char* msg, int length)
{
	char* ret = NULL;
	CHECK(NULL!=msg);
	CHECK(length>0);
	CHECK(NULL!=venda);
	memset(msg,0,length);
	if (venda->quantity>venda->qtdMax) {
		strncpy(msg,"Quantidade maior do que permitido",length);		
	} else {
		char* pdata;
		int len;
		char buf[BUFSIZE];
		strncpy(msg,"Problema na venda. Consulte seu fornecedor",length);		
		SCREEN_BANNER("Conectando para venda...");
		if (OK0==http_vget(buf, BUFSIZE, &pdata, &len, VENDA1_URL, venda->host, 
			venda->sn,venda->pdvId,venda->op)) {		
				if (NULL!=pdata) {
					int error;
					char scanbuf[BUFSIZE];					
					memset(scanbuf,0,BUFSIZE);
					memset(vendaGUID,0,BUFSIZE);
					strncpy(scanbuf,pdata,MIN(len,BUFSIZE));
					sscanf(scanbuf,"Erro=%d",&error);
					if (!error) {
						sscanf(scanbuf,"Erro=%d&GUID=%s",&error,vendaGUID);
						ret = vendaGUID;
					}					
				}								
		}
		if (NULL!=pdata) 
			free(pdata);
	}
	return ret;
}

vendaReply_t* vendaReplyNew()
{
	vendaReply_t* ret = (vendaReply_t*)calloc(1,sizeof(vendaReply_t));
	return ret;
}

void vendaReplyFree(vendaReply_t* reply) 
{
	if (NULL!=reply) {
		if (NULL!=reply->url) 
			free(reply->url);
		if (NULL!=reply->userMsg) 
			free(reply->userMsg);
		if (NULL!=reply->data)
			free(reply->data);
		if (NULL!=reply->hora)
			free(reply->hora);
		if (NULL!=reply->codigos) {
			int i;
			for (i=0;i<reply->quantity;i++) {
				free(reply->codigos[i]);
			}
			free(reply->codigos);
		}
		if (NULL!=reply->valorBase)
			free(reply->valorBase);
	}
}

static void getParameterInt(const char* url, int* value, const char* pattern)
{
	const char* ptr = strstr(url,pattern);
	if (NULL!=ptr) {
		char buf[BUFSIZE];
		_snprintf(buf,BUFSIZE,"%s%%d",pattern);
		sscanf(ptr,buf,value);
	}
}

static char* getParameterStr(const char* url, int length, const char* pattern)
{
	char* ptr, *parameter=NULL;	
	ptr = strstr(url,pattern);
	if (NULL!=ptr) {
		char buf[BUFSIZE];
		char* end;
		parameter = (char*)calloc(1,length+1);
		_snprintf(buf,BUFSIZE,"%s%%s",pattern);
		sscanf(ptr,buf,parameter);
		end = strchr(parameter,'&');
		if (NULL!=end) {
			*end = '\0';
		} else {
			end = strchr(parameter,';');
			if (NULL!=end) {
				*end = '\0';
			}
		}	
		parameter = removeMarkup(parameter);
	}
	return parameter;
}

vendaReply_t* processVendaReply(const char* data, int length)
{
	vendaReply_t* reply;
	char* end;
	CHECK(NULL!=data);	
	reply = vendaReplyNew();
	CHECK(NULL!=reply);
	reply->url = (char*)calloc(1,length+1);	
	memcpy(reply->url,data,length);
	getParameterInt(reply->url,&reply->esgotado,"Esgotar=");
	reply->userMsg = getParameterStr(reply->url,length,"Erro=");	
	getParameterInt(reply->url,&reply->pedidoId,"&PedidoId=");
	getParameterInt(reply->url,&reply->quantity,"&Qtd=");
	reply->data = getParameterStr(reply->url,length,"&Data=");
	reply->hora = getParameterStr(reply->url,length,"&Hora=");
	reply->hasError = (*reply->userMsg=='\0')? 0:1;
	// Now read the codes
	if (reply->quantity>0 && !reply->hasError) {
		int i;
		reply->codigos = (char**)calloc(reply->quantity,sizeof(char*));
		for (i=0,end=reply->url;i<reply->quantity;i++,end+=13) {
			end = strchr(end,';');
			if (NULL!=end) {
				end++;
				reply->codigos[i] = (char*)calloc(1,14); /*13 digits + '\0'*/
				memcpy(reply->codigos[i],end,13);
			}
		}
	}	
	return reply;
}

// \return reply must be freed
vendaReply_t* efetuaVenda(venda_t* venda, ingresso_t* ingresso, const char* guid)
{
	double valorBase=0,taxaAdm=0;
	char* pdata;
	int len;
	char buf[BUFSIZE],qtdBuf[INPUTSIZE],valorBuf[INPUTSIZE],taxaBuf[INPUTSIZE];
	vendaReply_t* reply = NULL;
	CHECK(NULL!=ingresso);
	CHECK(NULL!=venda);
	sprintf(qtdBuf,"%.02f",venda->quantity);	
	taxaAdm = atof(ingresso->taxaAdm);
	/* Maycon 26/07/2013
		Sobre o valor do ingresso, nуo diminua o valor da taxa
	*/
	//valorBase = atof(ingresso->valorv) - taxaAdm;
	valorBase = atof(ingresso->valorv);
	if (0==strcmp(ingresso->tipo,"M")) {
		valorBase /= 2.;
	}
	sprintf(valorBuf,"%.02f",valorBase);
	sprintf(taxaBuf,"%.02f",taxaAdm);
	if (OK0==http_vget(buf, BUFSIZE, &pdata, &len, VENDA2_URL, venda->host, 
		ingresso->id,ingresso->loteid,qtdBuf,venda->vista?"v":"p",
		valorBuf,ingresso->tipo,venda->lugar==NULL?"":venda->lugar,guid,taxaBuf,venda->sn,venda->pdvId,venda->op)) {
		if (NULL!=pdata) {
			reply = processVendaReply(pdata, len);
			ingresso->esgotado = reply->esgotado;
			reply->valorBase = strdup(valorBuf);
		}		
	}
	if (NULL!=pdata)
		free(pdata);		
	return reply;
}

#ifdef _CUTEST

static dict_t* getDumpIngressos(CuTest* tc);

static void fillDummyVenda(venda_t* venda, double quantidade)
{
	venda->host = "189.113.164.245";
	venda->op = "1125";
	venda->pdvId = "1135";
	venda->sn = "528-900-899";
	venda->qtdMax = 10;
	venda->vista = 1;
	venda->lugar = "0";
	venda->quantity = quantidade;
}

void test_testVenda(CuTest* tc) 
{
	char buf[BUFSIZE];
	venda_t venda;
	fillDummyVenda(&venda, 1);	
	CuAssertTrue(tc,NULL!=testVenda(&venda,buf,BUFSIZE));
	venda.quantity = 11;
	CuAssertTrue(tc,NULL==testVenda(&venda,buf,BUFSIZE));
}

void test_processVendaReply1(CuTest* tc)
{
	vendaReply_t* reply;
	char* data = "Erro=Lote%20esgotado.&Esgotar=1;¤¤¤¤ллллллллю■ю■ю■ю■ю■ю■";
	reply = processVendaReply(data,strlen(data));
	CuAssertTrue(tc,0==strcmp("Lote esgotado.",reply->userMsg));
	CuAssertTrue(tc,reply->esgotado);
	vendaReplyFree(reply);
}

void test_processVendaReply2(CuTest* tc)
{
	vendaReply_t* reply;
	char* data = "Erro=Lugar%20n%3Fo%20encontrado.;¤¤¤¤лллллллл■ю■";
	reply = processVendaReply(data,strlen(data));
	CuAssertTrue(tc,reply->hasError);
	CuAssertTrue(tc,0==strcmp("Lugar nуo encontrado.",reply->userMsg));
	vendaReplyFree(reply);
}

void test_processVendaReply3(CuTest* tc)
{
	vendaReply_t* reply;
	char* data = "Erro=&PedidoId=828615&Qtd=1&Data=16%2F03%2F2013&Hora=15%3A58%3A31;0114086593308¤¤¤¤лллллллл■ю■ю■";
	reply = processVendaReply(data,strlen(data));
	CuAssertTrue(tc,!reply->hasError);
	CuAssertTrue(tc,828615==reply->pedidoId);
	CuAssertTrue(tc,1==reply->quantity);
	CuAssertTrue(tc,0==strcmp(reply->data,"16/03/2013"));
	CuAssertTrue(tc,0==strcmp(reply->hora,"15:58:31"));
	vendaReplyFree(reply);
}

void confirmLoteEsgotado(CuTest* tc, vendaReply_t* reply, ingresso_t* ingresso)
{
	CuAssertTrue(tc,reply->hasError);
	CuAssertTrue(tc,0==strcmp("Lote esgotado.",reply->userMsg));
	CuAssertTrue(tc,reply->esgotado);
	CuAssertTrue(tc,ingresso->esgotado);
	CuAssertTrue(tc,NULL==reply->codigos);
}

void confirmLugarEsgotado(CuTest* tc, vendaReply_t* reply)
{
	CuAssertTrue(tc,reply->hasError);
	CuAssertTrue(tc,0==strcmp("Lugar nуo encontrado.",reply->userMsg));
	CuAssertTrue(tc,NULL==reply->codigos);
}

void confirmVendaOk(CuTest* tc, vendaReply_t* reply, double qtd)
{
	CuAssertTrue(tc,!reply->hasError);
	CuAssertTrue(tc,NULL!=reply->data);
	CuAssertTrue(tc,NULL!=reply->hora);
	CuAssertTrue(tc,qtd==reply->quantity);
	CuAssertTrue(tc,0<reply->pedidoId);
	CuAssertTrue(tc,NULL!=reply->codigos);
}

void test_efetuaVenda(CuTest* tc)
{
	venda_t venda;
	dict_t* ingressos = getDumpIngressos(tc);
	dictit_t it = dict_it_start();
	dictent_t ent;
	int i =0;
	double qtd = 2;
	CHECK(NULL!=ingressos);
	CHECK(0<ingressos->size);
	fillDummyVenda(&venda,qtd);	
	while (NULL!=(ent=dict_it_next(ingressos,it))) {
		char buf[BUFSIZE];
		const char* guid;
		vendaReply_t* reply;
		ingresso_t* ingresso = (ingresso_t*)ent->obj;
		CuAssertTrue(tc,NULL!=(guid=testVenda(&venda,buf,BUFSIZE)));
		CuAssertTrue(tc,NULL!=(reply=efetuaVenda(&venda,ingresso,guid)));
		switch(i++) {
			case 0:
				confirmLoteEsgotado(tc,reply,ingresso);
				break;
			case 1:
			case 2:
			case 3:
				confirmVendaOk(tc,reply,qtd);
				break;
			default:
				CuFail(tc,"more ingressos than expected");	
		}
		vendaReplyFree(reply);
	}
	dict_free(ingressos);
	dict_it_end(it);
}

CuSuite* CuGetVendaSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_testVenda);
	SUITE_ADD_TEST(suite, test_processVendaReply1);
	SUITE_ADD_TEST(suite, test_processVendaReply2);
	SUITE_ADD_TEST(suite, test_processVendaReply3);
	SUITE_ADD_TEST(suite, test_efetuaVenda);
	return suite;
}

dict_t* getDumpIngressos(CuTest* tc)
{
	dict_t* ret = NULL;
	const char* data, *dumpIngressos = 
		"5188aab8a13a244ccc3ce7e4e76ee21cCodigoBusca=2&EventoId=2&Id=12486&LoteId=2&Ingresso=20%2F05%20MARIA%20CEC%CDLIA%20%26%20RODOLFO%20-%20BACKSTAGE&Lote=1%BA%20LOTE&Sexo=U&Limitado=0&ValorV=150.00&ValorP=165.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=e3085513&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=3&EventoId=2&Id=12487&LoteId=2&Ingresso=20%2F05%20MARIA%20CEC%CDLIA%20%26%20RODOLFO%20-%20LOUNGE&Lote=1%BA%20LOTE&Sexo=U&Tipo=I&Limitado=0&ValorV=200.00&ValorP=220.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=9d1c4153&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=4&EventoId=2&Id=1010&LoteId=2&Ingresso=INGRESSO%20INDIVIDUAL&Lote=1%BA%20LOTE&Sexo=U&Tipo=I&Limitado=0&ValorV=1.00&ValorP=1.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=806180c0&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=4&EventoId=2&Id=1010&LoteId=1917&Ingresso=INGRESSO%20INDIVIDUAL&Lote=TERCEIRO%20LOTE&Sexo=U&Tipo=I&Limitado=0&ValorV=100.00&ValorP=120.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=806180c0&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0|CodigoBusca=1995&EventoId=2&Id=1&LoteId=2&Ingresso=PISTA%205&Lote=1%BA%20LOTE&Sexo=U&Tipo=I&Limitado=0&ValorV=0.00&ValorP=1.00&Esgotado=0&Lugar=0&ChaveImg=&QtdImg=0&ChaveModelo=9b1b4bc3&TaxaAdm=0.00&VendaFechada=1&Cupom=0&Vinculo=N&Multiplo=0";
	data = strdup(dumpIngressos);
	CuAssertTrue(tc,SUCCESS==createDictFromData(&ret, data));
	return ret;
}

#endif


