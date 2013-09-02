#ifndef __UTIL_H_
#define	__UTIL_H_

#include <stdio.h>
#ifndef _WIN32
#include <svc.h>
#else
#include <assert.h>
#endif

static char* const UPDATE_URL = "http://%s/posmanager/px73j3L0X2xa1/ServicosPOS/Atualizacao.php?a=b&d=%s&sn=%s";
static char* const VERIFY_URL = "http://%s/posmanager/px73j3L0X2xa1/ServicosPOS/Atualizacao.php?a=v&ce=%s&ci=%s&co=%s&ca=%s&sn=%s";
static char* const VENDA1_URL = "http://%s/posmanager/px73j3L0X2xa1/ServicosPOS/Teste.php?sn=%s&pdv=%s&op=%s";
static char* const VENDA2_URL = "http://%s/posmanager/px73j3L0X2xa1/ServicosPOS/Vender2.php?i=%s&l=%s&q=%s&m=%s&v=%s&t=%s&g=%s&guid=%s&x=%s&sn=%s&pdv=%s&op=%s";
static char* const MODELO_URL = "http://%s/posmanager/px73j3L0X2xa1/ServicosPOS/Modelos.php?id=%s&sn=%s";
static char* const RELATORIO1_URL = "http://%s/posmanager/px73j3L0X2xa1/RelatoriosPOS/Vendas2.php?d1=%s&d2=%s&hjxx=1&&sn=%s&op=%s&pdv=%s&evt=%s";
static char* const RELATORIO2_URL = "http://%s/posmanager/px73j3L0X2xa1/RelatoriosPOS/Vendas2.php?d1=%s&d2=%s&hjxx=1&&sn=%s&op=%s&pdv=%s&evt=%s&imp=1";

#define	CONFIG_LOWER_START	"/*"
#define	CONFIG_LOWER_END	"*/"

#define RT_CLOCK_TIME_LEN    15

extern char g_rtClock[];
extern int g_conHandle;

typedef enum {
	SUCCESS = 0,
	ERROR = -1
} ret_code;

// Limits
#define	MAX_DATASIZE				4096
#define	MTU							3200	// original 1200
#define MAX_VERSION_LENGTH			10
#define	MAX_DISPLAY_BUFFER			128
#define MAX_IP_LENGTH				20
#define MAX_DATASIZE				4096
#define	MAX_USERNAME_LENGTH			15
#define	MAX_PASSWORD_LENGTH			20
#define	MAX_PDVID_LENGTH			20
#define	MAX_PHONE_LENGTH			32
#define	MAX_MEDIA_NAME				10
#define	MAX_FILE_LENGTH				12
#define	MAX_APN_LENGTH				64
#define MAX_SERIAL_NUMBER_LENGTH    12
#define MAX_KEY_LENGTH              32
#define MAX_BUFF_SIZE				2048

// Config Defines
#define	CONFIG_DATASIZE				"#DATASIZE"
#define	CONFIG_HOSTIP				"#HOSTIP"
#define	CONFIG_PORT					"#PORT"
#define	CONFIG_PHONE				"#PHONE"
#define	CONFIG_USER					"#USERNAME"
#define CONFIG_PASSWORD				"#PASSWORD"
#define	CONFIG_APN					"#APN"
#define	CONFIG_SSL					"#SSL"
#define	CONFIG_PPPTYPE				"#PPPAUTHTYPE"
#define	CONFIG_CAFILE				"#CACERT"

#define	DEFAULT_IP					"0.0.0.0"
#define	DEFAULT_DATA_LEN			100
#define	DEFAULT_PORT				5003
#define	DEFAULT_USERPASS			"foobar"
#define	DEFAULT_PHONE				"203"
#define	DEFAULT_CACERT				"CACERT.PEM"
#define	DEFAULT_APN					"internet"

#define TEST_SN                     "528-900-899"
#define TEST_HOSTIP					"189.113.164.245"
#define TEST_PDVID                  "1135"

#define	KEY_0						0x30
#define	KEY_1						0x31
#define	KEY_2						0x32
#define	KEY_3						0x33
#define	KEY_4						0x34
#define	KEY_5						0x35
#define	KEY_6						0x36
#define	KEY_7						0x37
#define	KEY_8						0x38
#define	KEY_9						0x39
#define KEY_ESC						0x1B
#define KEY_BACK					0x08
#define KEY_ENTER					0xDB
#define KEY_HASH                    0x23
#define KEY_STAR                    0x2A

#define LOGO_FILENAME "logo.bmp"
#define LOGO_MONO_FILENAME "logo_mono.bmp"

#ifndef bool
typedef int   bool;
#define TRUE                1
#define FALSE               0
#endif

enum {TYPE_AMBIENTE = 0,
	  TYPE_OPERADORES,
	  TYPE_EVENTOS,
	  TYPE_INGRESSOS,
	  NUM_TYPES};

struct type_t
{
	char* filename;
	char* type;
	char* subs;
};

typedef struct {
   int    x;
   int    y;
} point;

extern struct type_t typetab[];

#define BLANK "                    "
#define SZ(X) strlen(X)
#define MIN(X,Y) ((X<Y)?(X):(Y))

#ifndef _WIN32
#define WRITE_AT(WHAT,X,Y) write_at(WHAT,SZ(WHAT),X,Y);
#define WRITE_AT_CENTERED(WHAT,X,Y) WRITE_AT(WHAT,X-SZ(WHAT)/2,Y);
#define WRITE(X, Y) LIMPA(Y); write_at(X, SZ(X), 0, Y), SVC_WAIT(200);
#define LIMPA(X) write_at(BLANK, SZ(BLANK), 0, X)
#define ABORT(X) clrscr(); { LIMPA(0); write_at(X, SZ(X), 0, 0); \
WaitForKeyPress(10); SVC_RESTART(NULL); }
#define CHECK(X) if (!(X)) {  char dbg[256]; clrscr(); \
		sprintf(dbg,"Fatal error at %d of file \"%s\".\n", __LINE__, __FILE__); ABORT(dbg); }
#ifndef _PTRDIFF_T_DEFINED
typedef int             ptrdiff_t;
#define _PTRDIFF_T_DEFINED
#endif
#else
#define CHECK(x) assert(x)
#define WRITE_AT(WHAT,X,Y) 
#endif

// Structures
typedef struct tagAPPCONFIG {
	short iDataSize;
	short iSSL;
	char szHostIP [MAX_IP_LENGTH];	
	int iPort;
	char szPhone [MAX_PHONE_LENGTH];
	char szUser [MAX_USERNAME_LENGTH];
	char szPassword [MAX_PASSWORD_LENGTH];
	char szAPN [MAX_APN_LENGTH];
	char szCAFile [MAX_FILE_LENGTH];
	char szSerialNr [MAX_SERIAL_NUMBER_LENGTH];
	int pppAuthType;
	
} appConfig_t;

extern appConfig_t g_AppConfig;

struct gprsConfig {
	const char* hostip;
	const char* apn;
//	const char* phone;
};

char convertKey(char cKey);
void convertLower(char * buffer,char * convertBuffer,int length);
char *strReplace(const char *orig, const char *rep, const char *with);
char* removeMarkup(char* text);
const char* chopWithElipsis(const char* text, int width);
void show_bmp(const char* filename);
void getScreenDims(point* top, point* bottom);

#ifndef _WIN32
void WaitForKeyPress (int row);
char *strdup (const char *s);
int getEnv(char* configVariable, char* configBuffer, int configBufferSize);
point getScreenCenter();
// After msecs it will exit (use msecs=-1 if there's no timeout)
void pressAnyKey(int row, int col,int msecs);
#define SCREEN_BANNER(X) clrscr(); { point center = getScreenCenter(); \
	write_at(X,SZ(X),center.x-SZ(X)/2,center.y); } 
#define SCREEN_WARNING(X) { SCREEN_BANNER(X); SVC_WAIT(2000); }
void loadGPRSConfig (void);
void saveGPRSConfig(struct gprsConfig* configs);
// Gets today in a null-terminated buffer of 11 bytes in the format dd/mm/yyyy
void getToday(char* buffer);
#else
#define SCREEN_BANNER(X)
#define SCREEN_WARNING(X) 
ret_code util_init(appConfig_t* appConfig);
char* util_read(const char* filename);
void loadGPRSConfig (void);
void saveGPRSConfig(struct gprsConfig* configs);
point getScreenCenter();
#endif

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetUtilSuite(void);
#endif

#endif	// __UTIL_H_
