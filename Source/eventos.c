#include "../Include/eventos.h"
#include "../Include/Util.h"
#include "../Include/trex.h"

static dict_t* eventos = NULL;
static ret_code ev_init();

dict_t* ev_getAll()
{
	CHECK(SUCCESS==ev_init());
	CHECK(NULL!=eventos);
	return eventos;
}

static ret_code ev_init()
{
	static int started = FALSE;
	ret_code ret = started? SUCCESS:ERROR;		
	int i = 1;
	if (!started) {
		const char* content;
		int length;
		eventos = dict_create();
		CHECK(NULL!=eventos);
		CHECK(SUCCESS==db_getContent(TYPE_EVENTOS,&content,&length));
		if (NULL!=content) {
			const TRexChar *begin, *end, *error;
			TRex* x;		
			CHECK(NULL!=(x=trex_compile(_TREXC("Id=(\\d+)&Evento=([\\c\\w\\s\\p]+)&"),&error)));
			while (trex_search(x,_TREXC(content),&begin,&end)) {
				int n = trex_getsubexpcount(x);	
				if (n==3) {
					char *id, *evento;					
					rx_getnext(x, 1, &id);
					rx_getnext(x, 2, &evento);
					CHECK(NULL!=id);
					CHECK(NULL!=evento);
					evento = removeMarkup(evento);
					/*clrscr();
					WRITE_AT(evento, 0, i++);
					SVC_WAIT(3000);*/
					dict_install(eventos, id, evento);						
					free(id);
					free(evento);
					ret = SUCCESS;
				}
				else {
					// Corrupted DB content?
					ret = ERROR;
					break;
				}
				content = end;
			}
			trex_free(x);
		}
		started = TRUE;
	}
	return SUCCESS;
}


