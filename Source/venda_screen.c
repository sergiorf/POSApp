#include "../Include/venda_screen.h"
#include "../Include/Util.h"
#include "../Include/menus.h"
#include "../Include/dict.h"
#include "../Include/trex.h"
#include "../Include/ingresso.h"
#include "../Include/ambiente.h"
#include "../Include/venda.h"
#include "../Include/operadores.h"
#include "../Include/eventos.h"
#include "../Include/keyboard.h"
#include "../Include/valor_utils.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAXBUF 100

static int h_clock = 0; // clock device handle

static dict_t* ingressos;
static ret_code ing_init();
static ret_code showDetails(ingresso_t* ingresso, int* y);

closure* vendaScreen(closure* cl)
{
	return showMenuEventos(ev_getAll(),telaIngresso,cl);
}

closure* telaIngresso(closure* cl)
{
	int n = 0;
	closure* next_cl=NULL;
	ingresso_t** data;
	CHECK(SUCCESS==ing_init());
	CHECK(NULL!=ingressos);
	data = getSortedIngressos(&n,ingressos,(const char*)cl->param);
	if (NULL!=data) {
		if (n>0) {
			point top, bottom;	
			MenuItem_t* items = (MenuItem_t*)calloc(n,sizeof(MenuItem_t));
			int i;
			getScreenDims(&top,&bottom);
			CHECK(NULL!=items);
			for (i=0;i<n;i++) {
				char* chopped = (char*)chopWithElipsis(data[i]->ingresso,bottom.x*.8);
				char details[MAXBUF];
				snprintf(details,MAXBUF,"\n%s, %s\0",ing_getSexDesc(data[i]),ing_getTipoDesc(data[i]));
				items[i].name = (char*)calloc(strlen(chopped)+strlen(details)+1,1);
				sprintf((char*)items[i].name,"%s%s",chopped,details);
				free(chopped);
				items[i].func = telaVenda;
				items[i].param = data[i];
			}
			next_cl = showMenu2(items,n,cl);
			while (--i>=0) {
				if (NULL!=items[i].name)
					free((void*)items[i].name);
			}
			free(items);
		}
		free(data);
	}
	if (NULL==next_cl) {
		SCREEN_WARNING("Não há mais ingressos para este evento");
		return popClosure(cl);
	} else {
		return next_cl;
	}
}

closure* telaVenda(closure* cl)
{
	ret_code result = ERROR;
	signed char q_buf[MAXBUF];	
	ingresso_t* ingresso;
	CHECK(NULL!=cl->param);
	ingresso = (ingresso_t*)cl->param;
	for (;SUCCESS!=result;) {		
		int y=3, ret_val;
		point top, bottom;	
		memset (q_buf,'\0',sizeof(q_buf));
		clrscr ();	
		CHECK(SUCCESS==set_cursor(1));		
		CHECK(SUCCESS==showDetails(ingresso,&y));
		y++;
		WRITE_AT("Quantidade:", 0, y);
		window (12, y, 18, y);		
		gotoxy (1, 1);
		ret_val = getkbd_entry (h_clock, "", (signed char *)q_buf, 0/*no timeout*/, NUMERIC,
                            (char *) szKeyMapVx680, sizeof (szKeyMapVx680), 10, 0);
		CHECK(SUCCESS==set_cursor(0));	
		clrscr();		
		getScreenDims(&top,&bottom);
		window(top.x,top.y,bottom.x,bottom.y);	
		if(-3==ret_val) {
			/* go to prev screen when user presses ESC*/
			result = SUCCESS;
		} else if (ret_val>0) {
			char msg[MAXBUF];
			venda_t venda;
			const char* vendaGUID;
			CHECK(SUCCESS==initAmbiente());
			venda.host = g_AppConfig.szHostIP;
			venda.op = g_operadorId;
			venda.pdvId = g_Ambiente.pdvId;
			venda.qtdMax = g_Ambiente.qtdMax;
			venda.sn = g_AppConfig.szSerialNr;
			venda.quantity = atof(q_buf);
			venda.lugar = NULL; /* to be looked at later */
			venda.vista = 1; /* always vista for the moment */
			if (NULL!=(vendaGUID=testVenda(&venda,msg,MAXBUF))) {
				vendaReply_t* reply;				
				SCREEN_BANNER("Efetuando venda...");
				reply = efetuaVenda(&venda,ingresso,vendaGUID);
				clrscr();
				if (NULL==reply || (reply->hasError && NULL==reply->userMsg)) {
					SCREEN_BANNER("Problema na venda, tente novamente");
				} else {
					if (reply->hasError) {
						SCREEN_BANNER(reply->userMsg);
					} else {
						SCREEN_BANNER("Imprimindo Ticket...");
						CHECK(SUCCESS==print_venda(&venda,ingresso,reply));
						SCREEN_BANNER("Venda efetuada com sucesso");
					}
				}
				vendaReplyFree(reply);
				pressAnyKey(2,1,5000);				
			} else {
				SCREEN_WARNING(msg);
			}
		}				
	}
	return popClosure(cl);
}

static ret_code showDetails(ingresso_t* ingresso, int* y)
{
	char buf[MAXBUF];
	int idx;
	dictent_t ent;
	dict_t* eventos = ev_getAll();
	CHECK(NULL!=ingresso);
	CHECK(NULL!=y);
	CHECK(NULL!=eventos);
	idx = *y;
	ent = dict_lookup(eventos, ingresso->eventoid);
	if (NULL!=ent) {
		WRITE_AT(ent->defn, 0, idx++);
	} else {
		WRITE_AT("Evento desconhecido", 0, idx++);
	}
	WRITE_AT(ingresso->ingresso, 0, idx++);
	idx++;
	snprintf(buf,MAXBUF,"Valor à vista: %s",valor_withCurrency(valor_format(ingresso->valorv)));
	WRITE_AT(buf, 0, idx++);
	snprintf(buf,MAXBUF,"Valor à prazo: %s",valor_withCurrency(valor_format(ingresso->valorp)));
	WRITE_AT(buf, 0, idx++);
	snprintf(buf,MAXBUF,"Taxa Administrativa: %s",valor_withCurrency(valor_format(ingresso->taxaAdm)));
	WRITE_AT(buf, 0, idx++);
	snprintf(buf,MAXBUF,"Sexo: %s",ing_getSexDesc(ingresso));		
	WRITE_AT(buf, 0, idx++);
	snprintf(buf,MAXBUF,"Tipo: %s",ing_getTipoDesc(ingresso));		
	WRITE_AT(buf, 0, idx++);
	*y = idx;
	return SUCCESS;
}

static ret_code ing_init()
{
	static int started = FALSE;
	ret_code ret = started? SUCCESS:ERROR;		
	int i = 1;
	if (!started) {
		char* content;
		int length;
		CHECK(SUCCESS==db_getContent(TYPE_INGRESSOS,&content,&length));
		ret = createDictFromData(&ingressos,content);		
		if (SUCCESS==ret)
			started = TRUE;
	}
	return SUCCESS;
}

