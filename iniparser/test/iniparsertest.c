#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iniparser.h"
#include "iniparsertest.h"

static void create_example_ini_file(void);
static int  parse_ini_file(char * ini_name);

void create_example_ini_file(void)
{
    FILE    *   ini ;
    ini = fopen("example.ini", "w");
    fprintf(ini,
    "#\n"
    "# This is an example of ini file\n"
    "#\n"
    "\n"
    "[Pizza]\n"
    "\n"
    "Ham       = yes ;\n"
    "Mushrooms = TRUE ;\n"
    "Capres    = 0 ;\n"
    "Cheese    = Non ;\n"
    "\n"
    "\n"
    "[Wine]\n"
    "\n"
    "Grape     = Cabernet Sauvignon ;\n"
    "Year      = 1989 ;\n"
    "Country   = Spain ;\n"
    "Alcohol   = 12.5  ;\n"
    "\n");
    fclose(ini);
}

void create_modelo_file(const char* ini_name) 
{
	FILE* ini = fopen(ini_name,"w");
	fprintf(ini,
		"[Report_1]\n"
		"align     = center ;\n"
		"fontsize  = 1 ;\n"
		"content   = {Codigo} - {PedidoId}\\n ;\n"
		"[Report_2]\n"
		"align     = center ;\n"
		"img       = l2ea ;\n"
		"[Report_3]\n"
		"align     = center ;\n"
		"img       = l2eb ;\n"
		"[Report_4]\n"
		"align     = center ;\n"
		"fontsize  = 2 ;\n"
		"content   = EVENTO DEMONSTRATIVO\\n\\nPISTA 5\\nUNISSEX -  {Tipo}\\nR$$ {Valor}\\n ;\n"
		"[Report_5]\n"
		"align     = center ;\n"
		"fontsize  = 1 ;\n"
		"content   = 1° EDIÇÃO - SERTANEJO NA VEIA\\n ;\n"
		"[Report_6]\n"
		"align     = center ;\n"
		"fontsize  = 1 ;\n"
		"barcode_value  = {Codigo} ;\n"
		"barcode_type   = I2of5 ;\n"
		"barcode_height = 96 ;\n"
		"barcode_width  = 2 ;\n"
		"content        = \\n{Codigo}\\n ;\n"
		"[Report_7]\n"
		"align     = center ;\n"
		"fontsize  = 1 ;\n"
		"content   = Visite:\\nwww.<b>TICPLUS</b>.com ;\n");
	fclose(ini);
}

int parse_ini_file(char * ini_name)
{
    dictionary  *   ini ;
    /* Some temporary variables to hold query results */
    int             b ;
    int             i ;
    double          d ;
    char        *   s ;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }
    iniparser_dump(ini, stderr);

    /* Get pizza attributes */
    printf("Pizza:\n");

    b = iniparser_getboolean(ini, "pizza:ham", -1);
    printf("Ham:       [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:mushrooms", -1);
    printf("Mushrooms: [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:capres", -1);
    printf("Capres:    [%d]\n", b);
    b = iniparser_getboolean(ini, "pizza:cheese", -1);
    printf("Cheese:    [%d]\n", b);

    /* Get wine attributes */
    printf("Wine:\n");
    s = iniparser_getstring(ini, "wine:grape", NULL);
    printf("Grape:     [%s]\n", s ? s : "UNDEF");
    
    i = iniparser_getint(ini, "wine:year", -1);
    printf("Year:      [%d]\n", i);

    s = iniparser_getstring(ini, "wine:country", NULL);
    printf("Country:   [%s]\n", s ? s : "UNDEF");
    
    d = iniparser_getdouble(ini, "wine:alcohol", -1.0);
    printf("Alcohol:   [%g]\n", d);

    iniparser_freedict(ini);
    return 0 ;
}

#ifdef _CUTEST

static int cmp_string(const char* a, const char* b)
{
	if (a==b) {
		return 0;
	} else if (a==NULL || b==NULL) {
		return (a==NULL)? -1:1;
	} else {
		return strcmp(a,b);
	}
}

void validate_modelo(CuTest* tc, const char * modelo_name)
{
	struct keys {
		char* align;
		int fontsize;
		char* content;
		char* img;
		char* barcode_value;
		char* barcode_type;
		int barcode_height;
		int barcode_width;
	} keytab[] = {
		"center",1,"{Codigo} - {PedidoId}\\n",NULL,NULL,NULL,-1,-1,
		"center",-1,NULL,"l2ea",NULL,NULL,-1,-1,
		"center",-1,NULL,"l2eb",NULL,NULL,-1,-1,
		"center",2,"EVENTO DEMONSTRATIVO\\n\\nPISTA 5\\nUNISSEX -  {Tipo}\\nR$$ {Valor}\\n",NULL,NULL,NULL,-1,-1,
		"center",1,"1° EDIÇÃO - SERTANEJO NA VEIA\\n",NULL,NULL,NULL,-1,-1,
		"center",1,"\\n{Codigo}\\n",NULL,"{Codigo}","I2of5",96,2,
		"center",1,"Visite:\\nwww.<b>TICPLUS</b>.com",NULL,NULL,NULL,-1,-1,
	};
	int numkeys = sizeof(keytab)/sizeof(keytab[0]);
	dictionary* ini = iniparser_load(modelo_name);
	int i;
	CuAssertTrue(tc,NULL!=ini);	
	for (i=0;i<numkeys;i++) {
		char buf[128];
		sprintf(buf,"report_%d:align\0",i+1);
		CuAssertTrue(tc,0==cmp_string(keytab[i].align,iniparser_getstring(ini,buf,NULL)));
		sprintf(buf,"report_%d:fontsize\0",i+1);
		CuAssertTrue(tc,keytab[i].fontsize==iniparser_getint(ini,buf,-1));
		sprintf(buf,"report_%d:content\0",i+1);	
		CuAssertTrue(tc,0==cmp_string(keytab[i].content,iniparser_getstring(ini,buf,NULL)));
		sprintf(buf,"report_%d:img\0",i+1);
		CuAssertTrue(tc,0==cmp_string(keytab[i].img,iniparser_getstring(ini,buf,NULL)));
		sprintf(buf,"report_%d:barcode_value\0",i+1);	
		CuAssertTrue(tc,0==cmp_string(keytab[i].barcode_value,iniparser_getstring(ini,buf,NULL)));
		sprintf(buf,"report_%d:barcode_type\0",i+1);	
		CuAssertTrue(tc,0==cmp_string(keytab[i].barcode_type,iniparser_getstring(ini,buf,NULL)));
		sprintf(buf,"report_%d:barcode_height\0",i+1);
		CuAssertTrue(tc,keytab[i].barcode_height==iniparser_getint(ini,buf,-1));
		sprintf(buf,"report_%d:barcode_width\0",i+1);
		CuAssertTrue(tc,keytab[i].barcode_width==iniparser_getint(ini,buf,-1));
	}
}		

void test_1stFile(CuTest* tc) 
{
	const char* modelo = "modelo.ini";	
	create_modelo_file(modelo);
	validate_modelo(tc,modelo);
}

CuSuite* CuGetIniParserSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_1stFile);
    return suite;
}

#endif