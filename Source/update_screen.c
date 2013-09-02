#include "../Include/update_screen.h"
#include "../Include/gprs.h"
#include "../Include/http_lib.h"
#include "../Include/db.h"
#include "../Include/dict.h"

#include <string.h>
#include <stdlib.h>

#define BUFSIZE 512

static const char* const MESSAGE_CONNECTING = "Conectando Ao Servidor";
static const char* const MESSAGE_CONNECTED = "Conectado Ao Servidor";
static const char* const MESSAGE_REGISTERING = "Registrando No Servidor";
static const char* const MESSAGE_DOWNLOADING = "Baixando Atualizações";
static const char* const MESSAGE_START = "Começando Aplicativo";

struct context_t
{
	point center;
	dict_t* keys;
	dict_t* needs_update;
	int star_count;
};

#define NUM_STARS 8 
char stars[][NUM_STARS] = {
	"-------",
	"*------",
	"**-----",
	"***----",
	"****---",
	"*****--",
	"******-",
	"*******"};	

extern int currSts;

static char tmpBuffer[BUFSIZE];

static ret_code loadAndStore(char* buf, struct context_t* context);
static ret_code verifyUpdate(char* buf, struct context_t* context);
static ret_code initContext(struct context_t* context, point center, dict_t* keys, dict_t* needs_update, int star_count);
static ret_code doProgress(struct context_t* context);

static ret_code loadAndStore(char* buf, struct context_t* context)
{	
	int i;
	CHECK(context!=NULL);
	CHECK(buf!=NULL);
	memset(buf,0,BUFSIZE);
	for (i=0; i<NUM_TYPES;i++) {
		dictent_t ent = dict_lookup(context->needs_update,typetab[i].type);
		CHECK(SUCCESS==doProgress(context));
		// Check if this type needs to be updated.
		if ((ent==NULL) || (*(ent->defn)=='0')) {
			char* pdata;
			int length;
			WRITE_AT("update",0,i+1);	
			CHECK(OK0==http_vget(buf,BUFSIZE,&pdata, &length, UPDATE_URL, g_AppConfig.szHostIP, 
				typetab[i].type, g_AppConfig.szSerialNr));		
			if (NULL!=pdata) {
				CHECK(SUCCESS==doProgress(context));
				CHECK(SUCCESS==db_Store(i, pdata, length));
				free(pdata);
			}
		}
		else {
			WRITE_AT("no need to update",0,i+1);
			SVC_WAIT(1000);
		}
	}
	return SUCCESS;
}

static ret_code verifyUpdate(char* buf, struct context_t* context)
{
	char *pdata, *ptr;
	dictent_t ent;
	int length, i, j;	

	CHECK(context!=NULL);
	CHECK(buf!=NULL);
	CHECK(SUCCESS==doProgress(context));
	CHECK(SUCCESS==db_getKeys(context->keys));	
	CHECK(SUCCESS==doProgress(context));

	memset(buf,0,BUFSIZE);
	CHECK(OK0==http_vget(buf, BUFSIZE, &pdata, &length, VERIFY_URL, g_AppConfig.szHostIP, 
		(ent=dict_lookup(context->keys,typetab[TYPE_EVENTOS].type))!=NULL?ent->defn:"",
		(ent=dict_lookup(context->keys,typetab[TYPE_INGRESSOS].type))!=NULL?ent->defn:"",
		(ent=dict_lookup(context->keys,typetab[TYPE_OPERADORES].type))!=NULL?ent->defn:"",
		(ent=dict_lookup(context->keys,typetab[TYPE_AMBIENTE].type))!=NULL?ent->defn:"",
		g_AppConfig.szSerialNr));
	CHECK(NULL!=pdata);
	for (i=0;i<NUM_TYPES;i++) {
		char* subs = typetab[i].subs;
		CHECK(SUCCESS==doProgress(context));
		ptr = pdata;
		for (j=0;j<length;j++) {
			if (0==memcmp(ptr,subs,strlen(subs))) {
				char buf[10];
				ptr+=3; // advance to value, e.g. CA=0 or CA=1
				ptr[1]='\0';
				/*sprintf(buf,"%s-%d",ptr,i);
				WRITE_AT(buf,0,i);
				SVC_WAIT(5000);*/
				dict_install(context->needs_update,typetab[i].type,ptr);
				break;
			}
			ptr++;
		}
	}
	if (NULL!=pdata)
		free(pdata);
	return SUCCESS;
}

static ret_code initContext(struct context_t* context, point center, dict_t* keys, dict_t* needs_update, int star_count)
{
	memset(context,0,sizeof(context));
	context->center = center;
	context->keys = keys;
	context->needs_update = needs_update;
	context->star_count = star_count;
	return SUCCESS;
}

static ret_code doProgress(struct context_t* context)
{
	WRITE_AT(stars[context->star_count],context->center.x-SZ(stars[context->star_count])/2,context->center.y+1);
	context->star_count = (context->star_count + 1) % NUM_STARS;
	return SUCCESS;
}

ret_code updateScreenInit()
{
	dict_t* keys;
	dict_t* needs_update;
	int j, errp;
	char* filename;
	FILE fd;
	struct context_t context;
	point center;

	clrscr();
	center = getScreenCenter();
	WRITE_AT(MESSAGE_CONNECTING,center.x-SZ(MESSAGE_CONNECTING)/2,center.y);

	CHECK(SUCCESS==initContext(&context,center,NULL,NULL,0));
	if (GPRS_STS_CONNECTED!=currSts) {
		// 1st connect GPRS 
		GPRS_init(&errp);
	//	CHECK(errp==0 || errp==-1037);
		while(currSts!=GPRS_STS_CONNECTED) {
			GPRS_yield(&errp);
			CHECK(errp==0 || errp==-1037);
			CHECK(SUCCESS==doProgress(&context));
			SVC_WAIT(500);
		}
	}
	CHECK(currSts==GPRS_STS_CONNECTED);
	
	clrscr(); // clear screen before writing new msg
	WRITE_AT(MESSAGE_DOWNLOADING,center.x-SZ(MESSAGE_DOWNLOADING)/2,center.y);	
	CHECK(NULL!=(keys=dict_create()));
	CHECK(NULL!=(needs_update=dict_create()));
	CHECK(SUCCESS==initContext(&context,center,keys,needs_update,0));
	CHECK(SUCCESS==verifyUpdate(tmpBuffer, &context));
	CHECK(SUCCESS==loadAndStore(tmpBuffer, &context));
	
	clrscr(); // clear screen before writing new msg
	WRITE_AT(MESSAGE_START,center.x-SZ(MESSAGE_START)/2,center.y);	
	SVC_WAIT(500);
	
	dict_free(keys);
	dict_free(needs_update);
	return SUCCESS;
}

