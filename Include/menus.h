#ifndef __MENUS_H_
#define	__MENUS_H_

#include "Util.h"
#include "dict.h"

typedef struct closure closure; // forward decl
typedef closure*(*MenuFunc)(closure*);

#define PAGE_ITEMS 4

struct closure
{	
	MenuFunc func;
	const void* param;
	int page; // 1 page == PAGE_ITEMS menu items
	closure* prev;
	closure* next;
};

typedef struct
{
  char key;
  const char* name;
  const void* param;
  MenuFunc func;
  int  hidden;
} MenuItem_t;

closure* showMenu(const MenuItem_t* items, int numItems, closure* cl);
closure* showMenuWithTimeout(const MenuItem_t* items, int numItems, closure* cl, long msecs, char defKey);
closure* showMenu2(MenuItem_t* items, int numItems, closure* cl);
closure* pushClosure(closure* cl, MenuFunc f, const void* param);
closure* popClosure(closure* cl);
#ifndef WIN32
closure* showMenuEventos(dict_t* eventos, MenuFunc f, closure* cl);
#endif

#endif	// __MENUS_H_


