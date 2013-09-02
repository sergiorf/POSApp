#include "../Include/operadores.h"
#include "../Include/db.h"
#include "../Include/dict.h"
#include "../Include/trex.h"
#include "../Include/http_lib.h"

char* g_operador = NULL;
char* g_operadorId = NULL;

static dict_t* operadores;
static dict_t* senhas;

static const char* OP_USERUNKNOWN = "Usuário Desconhecido";
static const char* OP_WRONGPASSWORD = "Senha Incorreta";

#ifndef _WIN32
static ret_code op_init();
#endif

ret_code op_download(dict_t** operadores, dict_t** senhas)
{	
	char* pdata;
	int length;
	ret_code ret = ERROR;
#define BUFSIZE 512
	char buf[BUFSIZE];
	CHECK(OK0==http_vget(buf, BUFSIZE, &pdata, &length, UPDATE_URL, g_AppConfig.szHostIP, 
				typetab[TYPE_OPERADORES].type, g_AppConfig.szSerialNr));		
	if (NULL!=pdata) {
		if (length>0) {
			char* content = (char*)calloc(length+1,1);
			memcpy(content, pdata, length);
			ret = op_parse(operadores,senhas,content);
		}
		free(pdata);		
	}
	return ret; 
}

ret_code op_parse(dict_t** operadores, dict_t** senhas, const char* content)
{
	const TRexChar *begin, *end, *error;
	const char* cursor = content;
	TRex* x;
	ret_code ret = ERROR;
	CHECK(NULL!=content);
	CHECK(NULL!=operadores);
	CHECK(NULL!=senhas);
	*operadores = dict_create();
	*senhas = dict_create();
	CHECK(NULL!=(x=trex_compile(_TREXC("Id=([\\c\\w\\s\\p]+)&Operador=([\\c\\w\\s\\p]+)&Senha=(\\d+)"),&error)));
	while (trex_search(x,_TREXC(cursor),&begin,&end)) {
		int n = trex_getsubexpcount(x);	
		if (n==4) {
			char *id, *operador, *senha;
			rx_getnext(x, 1, &id);
			rx_getnext(x, 2, &operador);
			rx_getnext(x, 3, &senha);
			CHECK(NULL!=id);
			CHECK(NULL!=operador);
			CHECK(NULL!=senha);
			id = removeMarkup(id);
			operador = removeMarkup(operador);
			dict_install(*operadores, id, operador);
			dict_install(*senhas, id, senha);
			free(id);
			free(operador);
			free(senha);
			ret = SUCCESS;
		}
		else {
			// Corrupted DB content?
			ret = ERROR;
			break;
		}
		cursor = end;
	} /* trex_search */ 
	trex_free(x);
	return ret;
}

#ifndef _WIN32

static ret_code op_init()
{
	static int started = FALSE;
	ret_code ret = started? SUCCESS:ERROR;		
	if (!started) {
		const char* content;
		int length;
		CHECK(SUCCESS==db_getContent(TYPE_OPERADORES,&content,&length));
		CHECK(SUCCESS==op_parse(&operadores,&senhas,content));
		started = TRUE;
		ret = SUCCESS;
	}
	return ret;
}

ret_code op_checkId(const char* id, const char* pwd, const char** err)
{
	dictent_t ent;
	ret_code ret = ERROR;
	CHECK(SUCCESS==op_init());
	CHECK(NULL!=id);
	ent = dict_lookup(senhas, id);
	if (NULL==ent) {
		*err = OP_USERUNKNOWN;
	} else {
		if (NULL!=pwd) {
			if (0!=strcmp(ent->defn,pwd)) {
				*err = OP_WRONGPASSWORD;
			} else ret = SUCCESS;
		} else ret = SUCCESS;		
	}
	if (SUCCESS==ret) {
		if (NULL!=g_operador)
				free(g_operador);	
		ent = dict_lookup(operadores, id);
		CHECK(ent!=NULL);
		g_operadorId = ent->name;
		g_operador = strdup(ent->defn); // why strdup?
		CHECK(NULL!=g_operador);
	}
	return ret;
}

#endif

#ifdef _CUTEST
void test_downloadOperadores(CuTest* tc) 
{
	dict_t* operadores, *senhas;
	CuAssertTrue(tc,SUCCESS==op_download(&operadores,&senhas));
	dict_free(operadores);
	dict_free(senhas);	
}

void test_parseOperadores(CuTest* tc)
{
	struct op_inf {
		const char* id;
		const char* name;
		const char* senha;
	} optab[] = {
		"1125","TESTES TICPLUS","0"
	};
	dict_t* operadores, *senhas;
	dictit_t it = dict_it_start();
	dictent_t ent;
	int i;
	const char* content = "add3db957e7c3e3d2d5d039a8618f471Id=1125&Operador=TESTES%20TICPLUS&Senha=0";
	CuAssertTrue(tc,SUCCESS==op_parse(&operadores,&senhas,content));
	i = 0;
	while (NULL!=(ent=dict_it_next(operadores,it))) {
		CuAssertTrue(tc,0==strcmp(optab[i].id,ent->name));
		CuAssertTrue(tc,0==strcmp(optab[i].name,ent->defn));
		i++;
	}
	dict_it_end(it);
	it = dict_it_start();
	i = 0;
	while (NULL!=(ent=dict_it_next(senhas,it))) {
		CuAssertTrue(tc,0==strcmp(optab[i].id,ent->name));
		CuAssertTrue(tc,0==strcmp(optab[i].senha,ent->defn));
		i++;
	}
	dict_it_end(it);
}

CuSuite* CuGetOperadoresSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_downloadOperadores);
	SUITE_ADD_TEST(suite, test_parseOperadores);
	return suite;
}
#endif



