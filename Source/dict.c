#include "../Include/dict.h"
#include "../Include/Util.h"

#include <string.h>
#include <stdlib.h>

typedef struct 
{
	int pos;
	int posinbucket;
} iterator_t;

#define HASHSIZE 101

/* hash: form hash value for string s */
static unsigned hash(const char *s, int hashsize)
{
    unsigned hashval;
    for (hashval = 0; *s != '\0'; s++)
      hashval = *s + 31 * hashval;
    return hashval % hashsize;
}

dict_t* dict_create()
{
	return dict_create2(HASHSIZE);
}

dict_t* dict_create2(int hashsize)
{
	dict_t* newdict = (dict_t*)calloc(1,sizeof(dict_t));
	CHECK(NULL!=newdict);
	CHECK(hashsize>0);
	newdict->size = 0;
	newdict->hashsize = hashsize;
	newdict->entries = (dictent_t*)calloc(hashsize,sizeof(dictent_t));
	return newdict;
}

void dict_free(dict_t* dict)
{
	if (dict != NULL) {
		int i;
		for (i=0; i<dict->hashsize; i++) {
			dictent_t np;
			for (np = dict->entries[i]; np != NULL; ) {
				dictent_t old = np;
				np = np->next;
				if (NULL!=old->defn) free(old->defn);
				if (NULL!=old->obj) {
					// only cleanup if user wanted to
					if (NULL!=dict->cleanup) dict->cleanup(old->obj);
				}
				free(old);
			}
		}
		free(dict->entries);
		free(dict);
	}
}

dictent_t dict_getfirst(dict_t* dict)
{
	dictent_t np = NULL;
	int i;
	if (dict == NULL)
		return NULL;
	for (i=0; i<dict->hashsize; i++) {
		np = dict->entries[i];
		if (NULL!=np) break;
	}
	return np;
}

/* lookup: look for s in hashtab */
dictent_t dict_lookup(dict_t* dict, const char *s)
{
    dictent_t np;
	if (dict == NULL)
		return NULL;
	for (np = dict->entries[hash(s,dict->hashsize)]; np != NULL; np = np->next)
        if (strcmp(s, np->name) == 0)
			return np; /* found */
    return NULL; /* not found */
}

/* install: put (name, defn) in hashtab */
void dict_install(dict_t* dict, char *name, const char *defn)
{
	dict_install2(dict, name, defn, NULL);
}

/* obj needs not be malloc'd user decides what to do in the cleanup function */
void dict_install2(dict_t* dict, char *name, const char *defn, void* obj)
{
    dictent_t np;
    unsigned hashval;
	CHECK(NULL!=dict);
    if ((np = dict_lookup(dict, name)) == NULL) { /* not found */
        np = (dictent_t) calloc(1,sizeof(struct nlist));
        if (np == NULL || (np->name = strdup(name)) == NULL)
          CHECK(0);
		hashval = hash(name,dict->hashsize);
        np->next = dict->entries[hashval];
        dict->entries[hashval] = np;
		dict->size++;
    } else { /* already there */
        if (NULL!=np->defn) free(np->defn); 
		if (NULL!=np->obj && obj!=np->obj) { 
			// only cleanup if user wanted to
			if (NULL!=dict->cleanup) dict->cleanup(np->obj);
		}
	}
	np->obj = obj;
	if ((np->defn = strdup(defn)) == NULL)
		CHECK(0);
    return;
}

dictit_t dict_it_start()
{
	return (dictit_t)calloc(1,sizeof(iterator_t));
}

dictent_t dict_it_next(dict_t* dict, dictit_t it)
{
	dictent_t next = NULL; 
	CHECK(it!=NULL);
	if (dict != NULL) {
		iterator_t* it2 = (iterator_t*)it;
		dictent_t np = dict->entries[it2->pos];
		int i;
		for (i=0;NULL!=np&&i<it2->posinbucket;np=np->next,i++);
		if (NULL!=np) {
			it2->posinbucket++;
			next = np;
		} else {
			for (it2->pos++; it2->pos<dict->hashsize; it2->pos++) {
				np = dict->entries[it2->pos];
				if (NULL!=np) {
					it2->posinbucket = 1;
					next = np;
					break;
				}
			}
		}
	}
	return next;
}

void dict_it_end(dictit_t it)
{
	if (it!=NULL)
		free(it);
}

void** dict_toObjArray(dict_t* dict)
{		
	void** result = NULL;
	if (NULL!=dict) {
		dictit_t it = dict_it_start();
		dictent_t ent;
		int index = 0;
		result = (void**)calloc(dict->size,sizeof(void*));
		while (NULL!=(ent=dict_it_next(dict,it))) {
			result[index++] = ent->obj;
		}
		dict_it_end(it);
	}
	return result;
}

#ifdef _CUTEST

void test_dictCreate(CuTest* tc)
{	
	dict_t* newdict = dict_create();
	CuAssertTrue(tc,NULL!=newdict);
	CuAssertTrue(tc,0==newdict->size);
	dict_free(newdict);
}

void test_dictInstall(CuTest* tc)
{	
	dict_t* newdict = dict_create();
	char* a="id",*b="value",*c="valuec";
	char* aa="id2",*bb="value2";
	dict_install(newdict,a,b);
	CuAssertTrue(tc,1==newdict->size);
	dict_install(newdict,a,b);
	CuAssertTrue(tc,1==newdict->size);
	dict_install(newdict,a,c);
	CuAssertTrue(tc,1==newdict->size);
	dict_install(newdict,aa,bb);
	CuAssertTrue(tc,2==newdict->size);
	dict_free(newdict);
}

void test_cleanupObj(void* obj)
{
	if (NULL!=obj)
		free(obj);
}

void test_dictInstallObj(CuTest* tc)
{	
	dict_t* newdict = dict_create();
	char* a="id",*b="value";
	dictent_t ent;
	typedef struct {
		int ii;
		int jj;
	} TObj;
	TObj* obj = (TObj*)malloc(sizeof(TObj));
	obj->ii = 98;
	obj->jj = 99;
	newdict->cleanup = test_cleanupObj;
	dict_install2(newdict,a,b,obj);
	CuAssertTrue(tc,1==newdict->size);
	dict_install2(newdict,a,b,obj);
	CuAssertTrue(tc,1==newdict->size);
	ent = dict_lookup(newdict, a);
	CuAssertTrue(tc,NULL!=ent);
	CuAssertTrue(tc,0==strcmp(ent->name,a));
	CuAssertTrue(tc,0==strcmp(ent->defn,b));
	obj = ent->obj;
	CuAssertTrue(tc,NULL!=obj);
	CuAssertTrue(tc,98==obj->ii);
	CuAssertTrue(tc,99==obj->jj);
	dict_free(newdict);
}

void test_dictLookup(CuTest* tc)
{
	dict_t* newdict = dict_create();
	char* a="id",*b="value",*c="valuec";
	dictent_t ent = dict_lookup(newdict, a);
	CuAssertTrue(tc,NULL==ent);
	dict_install(newdict,a,b);
	ent = dict_lookup(newdict, a);
	CuAssertTrue(tc,NULL!=ent);
	CuAssertTrue(tc,0==strcmp(ent->name,a));
	CuAssertTrue(tc,0==strcmp(ent->defn,b));
	dict_install(newdict,a,c);
	ent = dict_lookup(newdict, a);
	CuAssertTrue(tc,NULL!=ent);
	CuAssertTrue(tc,0==strcmp(ent->name,a));
	CuAssertTrue(tc,0==strcmp(ent->defn,c));
	dict_free(newdict);
}

void test_dictIterator(CuTest* tc)
{
	dict_t* newdict = dict_create();
	char* a="id",*b="value";
	dictit_t it = dict_it_start();
	dictent_t ent;
	dict_install(newdict,a,b);
	while (NULL!=(ent=dict_it_next(newdict,it))) {
		CuAssertTrue(tc,0==strcmp(ent->name,a));
		CuAssertTrue(tc,0==strcmp(ent->defn,b));
	}
	dict_it_end(it);
	dict_free(newdict);
} 

void test_dictIterator2(CuTest* tc)
{
	dict_t* newdict = dict_create2(1);
	char* a="id",*b="value";
	char* aa="id2",*bb="value2";
	dictit_t it = dict_it_start();
	dictent_t ent;
	int index = 0;
	dict_install(newdict,a,b);
	dict_install(newdict,aa,bb);
	CuAssertTrue(tc,2==newdict->size);
	while (NULL!=(ent=dict_it_next(newdict,it))) {
		if (index==0) {
			CuAssertTrue(tc,0==strcmp(ent->name,aa));
			CuAssertTrue(tc,0==strcmp(ent->defn,bb));
		} else {
			CuAssertTrue(tc,0==strcmp(ent->name,a));
			CuAssertTrue(tc,0==strcmp(ent->defn,b));
		}
		index++;
	}
	CuAssertTrue(tc,2==index);
	dict_it_end(it);
	dict_free(newdict);
}

void test_dictToObjArray(CuTest* tc)
{
	dict_t* newdict = dict_create();
	char* a="id",*b="value";
	dictent_t ent;
	typedef struct {
		int ii;
		int jj;
	} TObj;
	TObj** objArray;
	int i;
	TObj* obj = (TObj*)malloc(sizeof(TObj));
	obj->ii = 98;
	obj->jj = 99;
	newdict->cleanup = test_cleanupObj;
	dict_install2(newdict,a,b,obj);
	CuAssertTrue(tc,1==newdict->size);
	objArray = (TObj**)dict_toObjArray(newdict);
	CuAssertTrue(tc,NULL!=objArray);
	for(i=0;i<newdict->size;i++) {
		TObj* o = objArray[i];
		CuAssertTrue(tc,NULL!=o);
		CuAssertTrue(tc,o->ii==obj->ii);
		CuAssertTrue(tc,o->jj==obj->jj);
	}	
	free(objArray);
	dict_free(newdict);
}

CuSuite* CuGetDictSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_dictCreate);
	SUITE_ADD_TEST(suite, test_dictInstall);
	SUITE_ADD_TEST(suite, test_dictLookup);
	SUITE_ADD_TEST(suite, test_dictIterator);
	SUITE_ADD_TEST(suite, test_dictIterator2);
	SUITE_ADD_TEST(suite, test_dictInstallObj);
	SUITE_ADD_TEST(suite, test_dictToObjArray);
	return suite;
}

#endif
