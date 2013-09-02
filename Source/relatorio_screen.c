#include "../Include/relatorio_screen.h"
#include "../Include/eventos.h"
#include "../Include/Util.h"
#include "../Include/relatorio.h"
#include "../Include/ambiente.h"
#include "../Include/keyboard.h"
#include "../Include/date.h"
#include "../Include/valor_utils.h"
#include "../Include/write_utils.h"
#include "../Include/printer.h"
#include <string.h>
#include <stdlib.h>

#define MAXBUF 100
#define TIMEBUF 11

static closure* escolheTipoRelatorio(closure* cl);
static closure* telaEscolhePeriodo(closure* cl);
static closure* telaDiaria(closure* cl);
static closure* telaRelatorio(closure* cl, char* beginDate, char* endDate);
date_t* get_date(int x,int y,int size,const point* top, const point* bottom,int* clear_key_pressed);
static void show_data_incorreta(const point* bottom);

static int h_clock = 0; // clock device handle

closure* relatorioScreen(closure* cl)
{
	return showMenuEventos(ev_getAll(),escolheTipoRelatorio,cl);	
}

closure* escolheTipoRelatorio(closure* cl)
{	
	MenuItem_t items[] = {
		KEY_1, "Diária", NULL, telaDiaria, 0,
		KEY_2, "Por Período", NULL, telaEscolhePeriodo, 0,
	};   
	int i, n=sizeof(items)/sizeof(items[0]);
	for (i=0;i<n;i++) {
		items[i].param = cl->param;
	}	
	return showMenu2(items,n,cl);
}

closure* telaEscolhePeriodo(closure* cl)
{
	point top, bottom;
	int y,x;
	int clear_key_pressed = FALSE;
	date_t* begin =  NULL, *end = NULL;
	closure* next_cl = NULL;
	getScreenDims(&top, &bottom);	
	y=bottom.y/2-2;
	x=bottom.x/2-8;	
	while(!clear_key_pressed && NULL==next_cl) {
		clrscr ();	
		show_bmp(LOGO_FILENAME);
		WRITE_AT("Início: ddmmaaaa", x, y);		
		WRITE_AT("Fim:    ddmmaaaa", x, y+1);				
		begin = get_date(x+8,y,8,&top,&bottom,&clear_key_pressed);		
		if (NULL!=begin && !clear_key_pressed) {			
			end = get_date(x+8,y+1,8,&top,&bottom,&clear_key_pressed);		
		}
		if (!clear_key_pressed) {
			if (NULL==begin || NULL==end || !date_check2(begin,end)) {
				show_data_incorreta(&bottom);
			}  else if (NULL!=begin && NULL!=end) {
				next_cl = telaRelatorio(cl,date_makeCopy(begin),date_makeCopy(end));
			}
		}
		date_delete(begin);
		date_delete(end);
	}
	return next_cl? next_cl : popClosure(cl);
}

// Returns a date in the format dd/mm/yyyy
// The returned date must be freed
date_t* get_date(int x,int y,int size,const point* top,const point* bottom,int* clear_key_pressed)
{
	date_t* date = NULL;
	char buf[MAXBUF];	
	int ret_val;
	memset (buf,'\0',sizeof(buf));
	window(x,y,x+size,y);
	gotoxy(1,1);	
	CHECK(SUCCESS==set_cursor(1));		
	ret_val = getkbd_entry (h_clock, "", (signed char *)buf, 0/*no timeout*/, NUMERIC,
                            (char *) szKeyMapVx680, sizeof (szKeyMapVx680), 8, 8);
	window(top->x,top->y,bottom->x,bottom->y);	
	CHECK(SUCCESS==set_cursor(0));		
	if (ret_val>0) {
		CHECK(8==ret_val);
		date = date_new(buf);		
	} else if (-3==ret_val) {
		CHECK(NULL!=clear_key_pressed);
		*clear_key_pressed = TRUE;
	}
	return date;
}

void show_data_incorreta(const point* bottom)
{
	const char* msg = "Data Incorrecta";
	WRITE_AT(msg,bottom->x/2-strlen(msg)/2,bottom->y-1);
	SVC_WAIT(1000);
}

closure* telaDiaria(closure* cl)
{
	char today[TIMEBUF];
	getToday(today);
	return telaRelatorio(cl,strdup(today),strdup(today));
}

// *Date params will be deleted by this function
closure* telaRelatorio(closure* cl, char* beginDate, char* endDate)
{
	relatorio_t* relatorio;
	int isFinished = FALSE, refresh = TRUE; 
	char buf[MAXBUF];	
	const char* evId = (const char*)cl->param;
	point center = getScreenCenter(); 	
	memset (buf,'\0',sizeof(buf));
	snprintf(buf,MAXBUF,"Gerando relatório...");
	WRITE_AT(buf,center.x-10,center.y*2-1);
	CHECK(NULL!=evId);
	CHECK(SUCCESS==initAmbiente());
	CHECK(SUCCESS==get_relatorio(beginDate,endDate,evId,&relatorio));
	CHECK(NULL!=relatorio);	
	while (!isFinished) {
		char szKey;
		long lOSEvent = wait_event();
		if (refresh /*if there's no check it will keep rewriting constantly*/) {
			clrscr ();	
			show_bmp(LOGO_FILENAME);
			WRITE_AT_CENTERED("Relatorio",center.x,center.y-3);
			snprintf(buf,MAXBUF,"Inicio: %s\0",relatorio->begin_date);		
			WRITE_AT(buf,center.x-10,center.y);
			snprintf(buf,MAXBUF,"Fim:    %s\0",relatorio->end_date);		
			WRITE_AT(buf,center.x-10,center.y+1);
			snprintf(buf,MAXBUF,"Valor:  %s\0",valor_withCurrency(relatorio->value));
			WRITE_AT(buf,center.x-10,center.y+2);
			WRITE_AT("1> Imprimir",center.x-10,center.y+4);
			WRITE_AT("#> Tela Anterior",center.x-10,center.y+5);
			refresh = FALSE;
		} 
		if (lOSEvent & EVT_KBD) {
			if (read(g_conHandle, &szKey, 1) > 0) {
				szKey = convertKey (szKey);	
				if (KEY_HASH == szKey || KEY_BACK == szKey || KEY_ESC == szKey) {
					isFinished = TRUE;
				} else if (szKey == KEY_1) {
					char msg[MAX_BUFF_SIZE];
					if(relatorio_hasError(relatorio, msg, MAX_BUFF_SIZE)) {
						clrscr();
						write_multilineCentered(msg);
						SVC_WAIT(2000);
						refresh = TRUE;
					} else {
						CHECK(SUCCESS==print_relatorio(relatorio));
					}
				}
			}
		}
	}
	delete_relatorio(relatorio);
	return popClosure(cl);
}

