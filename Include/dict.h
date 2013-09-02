#ifndef __DICT_H_
#define	__DICT_H_

#include "Util.h"

struct nlist { /* table entry: */
    struct nlist *next; /* next entry in chain */
    char *name; /* defined name */
    char *defn; /* replacement text */
	void *obj; /* user defined object */
};

typedef struct nlist* dictent_t;
typedef void(*CleanupFunc)(void* obj);

typedef struct 
{
	int size;
	int hashsize;
	dictent_t* entries;
	CleanupFunc cleanup;
} dict_t;

typedef void* dictit_t;

dict_t* dict_create();
dict_t* dict_create2(int hashsize);
void dict_free(dict_t* dict);
dictent_t dict_lookup(dict_t* dict, const char *s);
void dict_install(dict_t* dict, char *name, const char *defn);
void dict_install2(dict_t* dict, char *name, const char *defn, void* obj);
dictit_t dict_it_start();
dictent_t dict_it_next(dict_t* dict, dictit_t it);
void dict_it_end(dictit_t it);
void** dict_toObjArray(dict_t* dict);
dictent_t dict_getfirst(dict_t* dict);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetDictSuite(void);
#endif

#endif	// __DICT_H_

