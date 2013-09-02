#include "../Include/modelo.h"
#include "../Include/http_lib.h"
#include "../Include/Util.h"
#include "../Include/dict.h"
#include "../Include/trex.h"
#include <stdlib.h>
#include <string.h>

static modelo_t* modelo_create(int num_reports);
static void modelo_getalignment(report_t* report, char* alignment);
static char* modelo_capture(const char* regexp, const char* begin_text, const char* end_text);
static void modelo_getcontent(report_t* report, const char* begin_text);
static void modelo_getcodebar(report_t* report);
static void modelo_getbarcodetype(report_t* report, char* type);
static void modelo_getimage(report_t* report);
static int modelo_getasint(char* text);

ret_code modelo_download(const ingresso_t* ingresso, char** pdata)
{	
#define BUFSIZE 512
	char buf[BUFSIZE];
	int length;
	ret_code ret = (OK0==http_vget(buf, BUFSIZE, pdata, &length, MODELO_URL, 
		g_AppConfig.szHostIP, ingresso->id, g_AppConfig.szSerialNr))? SUCCESS:ERROR;
	if (NULL!=*pdata) {
		*pdata = (char*)realloc(*pdata,length+1);
		(*pdata)[length] = '\0';
	} else {
		ret = ERROR;
	}
	return ret;
}

ret_code modelo_download2(const ingresso_t* ingresso, modelo_t** modelo)
{
	char* data;
	ret_code ret = modelo_download(ingresso,&data);
	if  (SUCCESS==ret) {
		ret = modelo_parse(data,modelo);
	}
	return ret;
}

ret_code modelo_parse(char* data, modelo_t** modelo)
{
	const TRexChar *begin, *end, *error;
	TRex* x;	
	int num_reports = 0;
	CHECK(NULL!=modelo);
	*modelo = NULL;
	if (NULL!=data) {
		modelo_t* m;
		const char* tmp = data;
		CHECK(NULL!=(x=trex_compile(_TREXC("<report"),&error)));
		while (trex_search(x,_TREXC(tmp),&begin,&end)) {
			tmp = end;
			num_reports++;		
		} // while trex_search
		trex_free(x);
		m = modelo_create(num_reports);
		if (NULL!=m) {
			int i = 0;
			const char* tmp = data;
			CHECK(NULL!=(x=trex_compile(_TREXC
				("<report[\\w\\p\\c\\s]*>"),&error)));
			while (trex_search(x,_TREXC(tmp),&begin,&end) && i<num_reports) {
				modelo_getcontent(m->reports[i], end);
				modelo_getcodebar(m->reports[i]);
				modelo_getimage(m->reports[i]);
 				modelo_getalignment(m->reports[i], modelo_capture("align=\"([\\w\\p\\c\\s]+)\"",begin,end));
				m->reports[i]->fontsize = modelo_getasint(modelo_capture("width=\"(\\d+)\"",begin,end));								
				i++;
				tmp = end;
			} // while trex_search
			trex_free(x);
			*modelo = m;
		}		
		free(data);
	}
	return SUCCESS;
}

void modelo_getcontent(report_t* report, const char* begin_text)
{
	TRex* x;
	const TRexChar *begin, *end, *error;
	CHECK(NULL!=begin_text);
	CHECK(NULL!=report);
	CHECK(NULL!=(x=trex_compile(_TREXC("</report>"),&error)));
	if (trex_search(x,begin_text,&begin,&end)) {
		int n = trex_getsubexpcount(x);		
		if (n>0) {
			ptrdiff_t size = begin-begin_text;
			char* content = (char*)calloc(size+1,1);
			memcpy(content,begin_text,size);
			report->content = content;
		}
	}
	trex_free(x);
}

void modelo_getcodebar(report_t* report) 
{	
	CHECK(NULL!=report);
	if (NULL!=report->content) {
		TRex* x;
		const TRexChar *begin, *end, *error;
		CHECK(NULL!=report);
		CHECK(NULL!=(x=trex_compile(_TREXC("<barcode[\\w\\p\\c\\s]*>"),&error)));
		if (trex_search(x,report->content,&begin,&end)) {
			int n = trex_getsubexpcount(x);
			if (n>0) {
				ptrdiff_t diff = (end-begin);
				size_t size = strlen(report->content)-diff;
				char* newcontent = (char*)calloc(size+1,1);
				memcpy(newcontent,end,size);
				modelo_getbarcodetype(report,modelo_capture("type=\"([\\w\\p\\c\\s]+)\"",begin,end));
				report->barcode_heigth = modelo_getasint(modelo_capture("height=\"(\\d+)\"",begin,end));
				report->barcode_width = modelo_getasint(modelo_capture("width=\"(\\d+)\"",begin,end));
				report->barcode_value = modelo_capture("value=\"([\\w\\p\\c\\s]+)\"",begin,end);
				report->report_type = BARCODE;
				free(report->content);
				report->content = newcontent;
			}
		}
	}
}

void modelo_getimage(report_t* report) 
{	
	CHECK(NULL!=report);
	if (NULL!=report->content) {
		TRex* x;
		const TRexChar *begin, *end, *error;
		CHECK(NULL!=report);
		CHECK(NULL!=(x=trex_compile(_TREXC("<img[\\w\\p\\c\\s]*>"),&error)));
		if (trex_search(x,report->content,&begin,&end)) {
			int n = trex_getsubexpcount(x);
			if (n>0) {
				report->image_id = modelo_capture("img src=\"([\\w\\p\\c\\s]+)\"",begin,end);
				report->report_type = IMAGE;
			}
		}
	}
}

void modelo_getbarcodetype(report_t* report, char* type)
{
	if (NULL!=type) {
		if (0==strncmp("I2of5",type,5)) {
			report->barcode_type = I2OF5;
		} else {
			report->barcode_type = UNKNOWN;
		}
		free(type);
	}
}

char* modelo_capture(const char* regexp, const char* begin_text, const char* end_text)
{
	TRex* x;
	char* ret = NULL;
	const TRexChar *begin, *end, *error;
	CHECK(NULL!=(x=trex_compile(_TREXC(regexp),&error)));
	if (trex_searchrange(x,begin_text,end_text,&begin,&end)) {
		int n = trex_getsubexpcount(x);			
		if (n>1) rx_getnext(x,1,&ret);
	}
	trex_free(x);
	return ret;
}

void modelo_getalignment(report_t* report, char* alignment)
{
	if (NULL!=alignment) {
		if (0==strncmp("center",alignment,6)) {
			report->alignment = CENTER;
		} else if (0==strncmp("left",alignment,4)) {
			report->alignment = LEFT;
		} else if (0==strncmp("right",alignment,5)) {
			report->alignment = RIGHT;
		} else report->alignment = NO;
		free(alignment);
	} else {
		report->alignment = NO;
	}
}

int modelo_getasint(char* text) 
{
	int val = -1;
	if (NULL!=text) {
		val = atoi(text);
		free(text);
	} 
	return val;
}

modelo_t* modelo_create(int num_reports) 
{
	modelo_t* modelo = NULL;
	if (num_reports>0) {
		int i;
		modelo = (modelo_t*)calloc(1,sizeof(modelo_t));
		modelo->num_reports = num_reports;
		modelo->reports = (report_t**)calloc(num_reports,sizeof(report_t*));
		for (i=0;i<num_reports;i++) {
			modelo->reports[i] = (report_t*)calloc(1,sizeof(report_t));
			modelo->reports[i]->report_type = TEXT;
			modelo->reports[i]->barcode_type = UNKNOWN;
		}
	}
	return modelo;
}

void modelo_delete(modelo_t* modelo)
{
	if (NULL!=modelo) {
		if (NULL!=modelo->reports) {
			int i;
			for (i=0;i<modelo->num_reports;i++) {
				if(NULL!=modelo->reports[i]->content)
					free(modelo->reports[i]->content);
				if(NULL!=modelo->reports[i]->barcode_value)
					free(modelo->reports[i]->barcode_value);
				if(NULL!=modelo->reports[i]->image_id)
					free(modelo->reports[i]->image_id);
				free(modelo->reports[i]);
			}
			free(modelo->reports);
		}
		free(modelo);
	}		
}

#ifdef _CUTEST

void test_modeloParse(CuTest* tc) 
{	
	report_t reports[] = {
		CENTER,1,"$(Codigo) - $(PedidoId)\n  <br/>\n  &nbsp;",TEXT,0,0,UNKNOWN,NULL,NULL,
		CENTER,-1,"\n  <img src=\"F:l2ea\" alt=\"Imagem E1\"/>\n",IMAGE,0,0,UNKNOWN,NULL,"F:l2ea",
		CENTER,2,"EVENTO DEMONSTRATIVO\n  <br/>\n  <br/>\n  PISTA 5\n  <br/>\n  UNISSEX -  $(Tipo)\n  <br/>\n  <b></b>\n  R$$ $(Valor)\n  <br/>\n  &nbsp;",TEXT,0,0,UNKNOWN,NULL,NULL,
		CENTER,1,"1° EDIÇÃO - SERTANEJO NA VEIA\n  <br/>\n  &nbsp;",TEXT,0,0,UNKNOWN,NULL,NULL,
		LEFT,1,"LOTE: $(Lote)\n  <br/>\n  PDV: $(Pdv)\n  <br/>\n  DATA: $(Data) $(Hora)\n  <br/>\n  &nbsp;",TEXT,0,0,UNKNOWN,NULL,NULL,
		CENTER,1,"\n  <br/>\n  $(Codigo)\n  <br/>\n  &nbsp;",BARCODE,2,96,I2OF5,"$(Codigo)",NULL,
		CENTER,1,"Visite:\n  <br/>\n  www.\n  <b>TICPLUS</b>\n  .com",TEXT,0,0,UNKNOWN,NULL,NULL,
		NO,-1,"\n  <br/>\n  <br/>\n  <br/>\n  <br/>\n  <br/>\n  <br/>\n  <br/>\n  <br/>\n",TEXT,0,0,UNKNOWN,NULL,NULL,
	};
	char* buffer = util_read("modelo.wml");
	modelo_t* modelo;
	int i;
	CuAssertTrue(tc,NULL!=buffer);
	CHECK(SUCCESS==modelo_parse(buffer,&modelo));
	CuAssertTrue(tc,8==modelo->num_reports);
	CuAssertTrue(tc,NULL!=modelo->reports);
	for (i=0;i<modelo->num_reports;i++) {
		CuAssertTrue(tc,reports[i].alignment==modelo->reports[i]->alignment);
		CuAssertTrue(tc,reports[i].barcode_heigth==modelo->reports[i]->barcode_heigth);
		CuAssertTrue(tc,reports[i].barcode_type==modelo->reports[i]->barcode_type);
		if (NULL==reports[i].barcode_value) {
			CuAssertTrue(tc,NULL==modelo->reports[i]->barcode_value);
		} else {
			CuAssertTrue(tc,0==strcmp(reports[i].barcode_value,modelo->reports[i]->barcode_value));
		}
		CuAssertTrue(tc,reports[i].barcode_width==modelo->reports[i]->barcode_width);
		CuAssertTrue(tc,0==strcmp(reports[i].content,modelo->reports[i]->content));
		CuAssertTrue(tc,reports[i].fontsize==modelo->reports[i]->fontsize);
		CuAssertTrue(tc,reports[i].report_type==modelo->reports[i]->report_type);
		if (NULL==reports[i].image_id) {
			CuAssertTrue(tc,NULL==modelo->reports[i]->image_id);
		} else {
			CuAssertTrue(tc,NULL!=modelo->reports[i]->image_id);
			CuAssertTrue(tc,0==strcmp(reports[i].image_id,modelo->reports[i]->image_id));
		}
	}
	modelo_delete(modelo);
}

void test_modeloDownload(CuTest* tc)
{
	dict_t* ingressos;
	dictent_t ent;
	dictit_t it = dict_it_start();
	CuAssertTrue(tc,SUCCESS==ing_download(&ingressos));	
	while (NULL!=(ent=dict_it_next(ingressos,it))) {
		char* data;
		modelo_t* modelo;
		CHECK(SUCCESS==modelo_download((const ingresso_t*)ent->obj,&data));
		CHECK(SUCCESS==modelo_parse(data,&modelo));
		modelo_delete(modelo);
	}
	dict_it_end(it);
	dict_free(ingressos);
} 

CuSuite* CuGetModeloSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_modeloParse);
	SUITE_ADD_TEST(suite, test_modeloDownload);
	return suite;
}

#endif

