#include "../Include/menus.h"
#include "../Include/Util.h"
#include "../Include/write_utils.h"

#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
#define EVT_KBD 0
#define EVT_TIMER 0
static int read(int* a,char* b,int c) {return 0;}
static long wait_event() {return 0;}
static long set_timer(long msecs, int evt) {return 0;}			
static void clrscr() {};
#endif

static ret_code printMenu(const MenuItem_t* items, int numItems);
static ret_code printMenu2(MenuItem_t* items, int numItems, bool multipage, bool lastpage, int page);

closure* showMenu2(MenuItem_t* items, int numItems, closure* cl)
{
	closure* next_cl = NULL;
	int isFinished = FALSE; 
	bool multipage, firstpage, lastpage;
	CHECK(NULL!=cl);
	firstpage = cl->page==0;
	multipage = (numItems > PAGE_ITEMS);
	lastpage = (cl->page+1)*PAGE_ITEMS >= numItems;
	CHECK(SUCCESS==printMenu2(items,numItems,multipage,lastpage,cl->page));
	while (!isFinished) {
		char szKey;
		long lOSEvent = wait_event();
		if (lOSEvent & EVT_KBD) {
			if (read(g_conHandle, &szKey, 1) > 0) {
				int i;
				const MenuItem_t* curr = items;
				szKey = convertKey (szKey);	
				if (KEY_HASH == szKey || KEY_BACK == szKey) {
					if (multipage && !firstpage) {
						cl->page--;
						next_cl = cl;
						isFinished = TRUE;
					} else {
						next_cl = popClosure(cl);
						isFinished = TRUE;
					}
				} else if (szKey == KEY_STAR) {
					if (multipage && !lastpage) {
						cl->page++;
						next_cl = cl;
						isFinished = TRUE;
					}
				} else {
					for (i=0; i<numItems; i++) {
						if (curr->key == szKey) {
							CHECK(NULL!=(curr->func));
							next_cl = pushClosure(cl, curr->func, curr->param);
							isFinished = TRUE;
							break;
						}
						curr++;
					}
				}
			}
		}
	}
	return next_cl;
}

closure* showMenu(const MenuItem_t* items, int numItems, closure* cl)
{
	return showMenuWithTimeout(items,numItems,cl,0,0);
}

closure* showMenuWithTimeout(const MenuItem_t* items, int numItems, closure* cl, long msecs, char defKey)
{
	closure* next_cl = NULL;
	int isFinished = FALSE; 
	CHECK(NULL!=cl);
	CHECK(SUCCESS==printMenu(items, numItems));
	while (!isFinished) {
		char szKey = 0;
		long lOSEvent;		
		if (msecs > 0) {
			CHECK(0<=set_timer(msecs, EVT_TIMER));
		}
		lOSEvent = wait_event();
		if (lOSEvent & EVT_KBD || lOSEvent & EVT_TIMER) {	
			if (lOSEvent & EVT_TIMER) {
				szKey = defKey;
			}	
			if (0!=szKey || read(g_conHandle, &szKey, 1) > 0) {
				int i;
				const MenuItem_t* curr = items;
				szKey = convertKey (szKey);				
				for (i=0; i<numItems; i++) {
					if (curr->key == szKey) {
						CHECK(NULL!=(curr->func));
						next_cl = pushClosure(cl, curr->func, NULL);
						isFinished = TRUE;
						break;
					}
					curr++;
				}
			}
			CHECK(SUCCESS==printMenu(items,numItems));
		} 	
	}
	return next_cl;
}

ret_code printMenu2(MenuItem_t* items, int numItems, bool multipage, bool lastpage, int page)
{
#define MAX 50
	point center = getScreenCenter(); 	
	int start, end, i, h, rowidx;
	clrscr();
	show_bmp(LOGO_FILENAME);
	CHECK(NULL!=items);
	CHECK(numItems>0);
	start = page * PAGE_ITEMS;
	if (multipage) {
		end = lastpage? numItems : (page+1)*PAGE_ITEMS;
	} else {
		end = numItems;
	}
	CHECK(end<=numItems);
	h = center.y - (end-start);
	items = items + start;
	for (rowidx=0, i=start; i<end+2; i++,rowidx++) {
		char buf[MAX];
		if (i==end) {
#ifdef WIN32
			_snprintf(buf,MAX,"#> %s","Tela Anterior");
#else
			snprintf(buf,MAX,"#> %s","Tela Anterior");
#endif
			WRITE_AT(buf,2,h+rowidx*2);
		
		} else if (multipage && !lastpage && i==(end+1)) {
#ifdef WIN32
			_snprintf(buf,MAX,"*> %s","Tela Seguinte");
#else
			snprintf(buf,MAX,"*> %s","Tela Seguinte");
#endif
			WRITE_AT(buf,2,h+rowidx*2);

		} else if (i<end) {
			items->key = KEY_1 + rowidx;
#ifdef WIN32
			_snprintf(buf,MAX,"%d> ",rowidx + 1);
#else
			snprintf(buf,MAX,"%d> ",rowidx + 1);
#endif		
			WRITE_AT(buf,2,h+rowidx*2);
			write_multiline(items->name,5,h+rowidx*2,2);

		} else {
			break;
		}
		items++;
	}
	return SUCCESS;
}

ret_code printMenu(const MenuItem_t* items, int numItems)
{
#define MAX 50
	point center = getScreenCenter(); 
	int i, j, h, k=0;
	clrscr();
	show_bmp(LOGO_FILENAME);
	CHECK(NULL!=items);
	CHECK(numItems>0);
	for (i=0;i<numItems;i++) {
		if (!items->hidden) {
			k++;
		}
	}
	h = center.y - k + 2;
	for (i=0, j=0; i<numItems; i++) {
		char buf[MAX];
		if (!items->hidden) {
#ifdef WIN32
			_snprintf(buf,MAX,"%d> %s",i+1,items->name);
#else
			snprintf(buf,MAX,"%d> %s",j+1,items->name);
#endif
			WRITE_AT(buf,4,h+(j++)*2);
		}
		items++;
	}
	return SUCCESS;
}

#ifndef WIN32
closure* showMenuEventos(dict_t* eventos, MenuFunc f, closure* cl)
{
	closure* next_cl;
	MenuItem_t* items; 
	dictit_t it = dict_it_start();
	dictent_t ent;
	int i = 0;
	point top, bottom;	
	CHECK(NULL!=eventos);	
	items = (MenuItem_t*)calloc(eventos->size,sizeof(MenuItem_t));
	CHECK(NULL!=items);
	getScreenDims(&top,&bottom);	
	while (NULL!=(ent=dict_it_next(eventos,it))) {		
		items[i].func = f;
		items[i].name = chopWithElipsis(ent->defn,bottom.x*.8);
		items[i].param = ent->name;
		i++;
	}	
	dict_it_end(it);
	next_cl = showMenu2(items,i,cl);
	while (--i>=0) {
		if (NULL!=items[i].name)
			free((void*)items[i].name);
	}
	free(items);
	return next_cl;
}
#endif

closure* pushClosure(closure* cl, MenuFunc f, const void* param)
{
	closure* next = (closure*)malloc(sizeof(closure));
	CHECK(NULL!=next);
	CHECK(NULL!=cl);
	cl->next = next;
	next->prev = cl;
	next->next = NULL;
	next->func = f;
	next->param = param;
	next->page = 0;
	return next;
}

closure* popClosure(closure* cl)
{
	closure* prev;
	CHECK(NULL!=cl);
	CHECK(NULL!=cl->prev);
	prev = cl->prev;
	prev->next = NULL;
	free(cl);
	return prev;
}
