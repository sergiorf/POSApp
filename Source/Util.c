#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>
#include "../Include/Util.h"

struct type_t typetab[] = {
	"AMBIENTE.db", "a", "CA",
	"OPERADORES.db", "o", "CO",
	"EVENTOS.db", "e", "CE",
	"INGRESSOS.db", "i", "CI",
};

#ifndef _WIN32
#include "../Include/keyboard.h"
char szKeyMapVx680[MAX_ALPNUM_KEYS][CHAR_PER_KEY_VX680] =
{
    "0- +%",    "1QZqz.\\",    "2ABCabc&",    "3DEFdef%",
    "4GHIghi*",    "5JKLjkl/",    "6MNOmno~",    "7PRSprs^",
    "8TUVtuv[",    "9WXYwxy]",    "*,'\":",    "#=:$?"
};
#else
static int screen_size(char* buffer) {
	buffer[0] = 50;
	buffer[1] = 50;
	return 0;
}
#endif

char convertKey(char cKey) 
{
    if((unsigned char) cKey > 0xf9) {
        cKey -= 0xf9;   // screen keys: fa-fd -> 1-4
    }
    else if((unsigned char) cKey < 0xef) {
        cKey &= 0x7f;   // keypad keys: 0-ef -> 0-7f
	}	
    return(cKey);
}

void getScreenDims(point* top, point* bottom)
{
	char buf[2]; 	
	CHECK(NULL!=top);
	CHECK(NULL!=bottom);
	CHECK(SUCCESS==(screen_size(buf)));
	top->x = 0;
	top->y = 0;
	bottom->y = ((int)buf[0]);
	bottom->x = ((int)buf[1]);
}

#ifndef _WIN32
void WaitForKeyPress (int row)
{
	char szKey;

	write_at ("PRESS ANY KEY        ", strlen ("PRESS ANY KEY        "), 1, row);

	for (;;) {
		if (wait_evt (EVT_KBD)) {
			read (g_conHandle, &szKey, 1);
			break;
		}
	}
}

char *strdup (const char *s) 
{
    char *d = (char*)calloc (strlen(s)+1,sizeof(char));   // Space for length plus nul
    if (d == NULL) return NULL;          // No memory
    strcpy (d,s);                        // Copy the characters
    return d;                            // Return the new string
}

/*
Retrieves config.sys data and parses the variable for lower case delimeters.
Converts data in lower case delimeters to lowere case
Lower case strings are delimited using C style comments
*/
int getEnv(char* configVariable, char* configBuffer, int configBufferSize)
{
    char configData[256];
    char convertedData[256];
    char * currentConvert = convertedData;
    char * tempStart;
    char * lowerStart;
    char * begin = NULL;
    char * lowerEnd = NULL;

    int size = get_env(configVariable, configData, sizeof(configData));

    if (size > 0) {
        begin = configData;
        configData[size] = 0;

        while ((begin!= NULL) && (begin[0] != 0)) {
            lowerStart = strstr(begin, CONFIG_LOWER_START);

			if (lowerStart != NULL) {
                memcpy(currentConvert, begin, (int)(lowerStart - begin));
                currentConvert += (int)(lowerStart - begin);
                *currentConvert = 0;

                /*
                Copy all the data up till the lower start to the lower case end sentinal string
                */
                tempStart = &lowerStart[2];
                lowerEnd = strstr(tempStart, CONFIG_LOWER_END);
                
				if (lowerEnd != NULL) {
                    convertLower(tempStart, currentConvert,(int) (lowerEnd - tempStart));
                    currentConvert += (int)(lowerEnd - tempStart);
                    *currentConvert = 0;
                    begin = &lowerEnd[2];
                } else {
                    /*
                    If no end delimeter string is found, then convert all remaining data
                    to lower case
                    */
                    convertLower(tempStart, currentConvert, strlen(tempStart));
                    currentConvert += strlen(tempStart);
                    *currentConvert = 0;
                    begin = 0;
                }
            } else {
                strcpy(currentConvert, begin);
                begin = 0;
            }
        }//while

        /*
        OK, now check if it safe to copy the coverted data to the buffer
        provided
        */
        size = strlen(convertedData);
        if (strlen(convertedData) <= configBufferSize) {
            memcpy(configBuffer, convertedData, size);
        }
    }
    return size;   
}

void loadGPRSConfig (void)
{
	char ip[MAX_IP_LENGTH+1], apn[MAX_APN_LENGTH+1];
	int size;
	if ((size=get_env(CONFIG_HOSTIP, ip, MAX_IP_LENGTH)) < 0) {
		strcpy (g_AppConfig.szHostIP, DEFAULT_IP);
	} else {
		ip[size] = '\0';
		strcpy (g_AppConfig.szHostIP, ip);
	}
	if ((size=get_env(CONFIG_APN, apn, MAX_APN_LENGTH)) < 0) {
		strcpy (g_AppConfig.szAPN, DEFAULT_APN);
	} else {
		apn[size] = '\0';
		strcpy (g_AppConfig.szAPN, apn);
	}
}

void saveGPRSConfig(struct gprsConfig* config)
{
	char text1[MAX_IP_LENGTH+3], text2[MAX_APN_LENGTH+4];
	point center = getScreenCenter();
	CHECK(NULL!=config);
	CHECK(0<put_env(CONFIG_HOSTIP,config->hostip,strlen(config->hostip)));
	//CHECK(0<put_env(CONFIG_PHONE,config->phone,strlen(config->phone)));
	CHECK(0<put_env(CONFIG_APN,config->apn,strlen(config->apn)));	
	sprintf(text1,"IP:%s",config->hostip);
	sprintf(text2,"APN:%s",config->apn);
	clrscr();
	WRITE_AT("Configurado com sucesso",center.x-SZ("Configurado com sucesso")/2,center.y-1);
	WRITE_AT(text1,center.x-SZ(text1)/2,center.y);
	WRITE_AT(text2,center.x-SZ(text2)/2,center.y+1);
	SVC_WAIT(2000);
}

// Gets today in a null-terminated buffer of 11 bytes in the format dd/mm/yyyy
void getToday(char* buffer)
{
#define CLOCKWIDTH 15
    int hClock;
    int size, c=0;
    char time[CLOCKWIDTH];
   
    /* Read clock and display result */
    hClock = open(DEV_CLOCK, O_RDWR);
    size = read(hClock, time, CLOCKWIDTH);
	CHECK(size == CLOCKWIDTH);

	buffer[c++] = time[6];
	buffer[c++] = time[7];
	buffer[c++] = '/';
	buffer[c++] = time[4];
	buffer[c++] = time[5];
	buffer[c++] = '/';
	buffer[c++] = time[0];
	buffer[c++] = time[1];
	buffer[c++] = time[2];
	buffer[c++] = time[3];
	buffer[c++] = '\0';
        
	close(hClock);
}

point getScreenCenter(void)
{
	point p;
	char buf[2]; 
	CHECK(SUCCESS==(screen_size(buf)));
	p.y = ((int)buf[0]) / 2;
	p.x = ((int)buf[1]) / 2;
	return p;
}

void pressAnyKey(int row, int col,int msecs) 
{	
	int timerId = -1;
	if (msecs > 0) {
		CHECK(0<=(timerId=set_timer(msecs, EVT_TIMER)));
	}
	WRITE_AT("APERTAR TECLA        ",row,col);
	for (;;) {
		long lOSEvent = wait_event ();
		if (lOSEvent & EVT_KBD || lOSEvent & EVT_TIMER) {
			char szKey;
			if ((lOSEvent & EVT_TIMER) || read (g_conHandle, &szKey, 1) > 0) {
				break;
			}
		}
	}
	if (msecs>0) {
		clr_timer(timerId);
	}
}

void show_bmp(const char* filename)
{
	set_display_coordinate_mode (PIXEL_MODE);	
	CHECK(0==put_BMP_at(0,0,(char*)filename));
	set_display_coordinate_mode (CHARACTER_MODE);	
}

#else

ret_code util_init(appConfig_t* appConfig)
{
	CHECK(NULL!=appConfig);
	strncpy (appConfig->szHostIP, TEST_HOSTIP, MAX_IP_LENGTH);
	strncpy (appConfig->szSerialNr, TEST_SN, MAX_SERIAL_NUMBER_LENGTH);
	return SUCCESS;
}

char* util_read(const char* filename)
{
	FILE *in;
	char* buffer = NULL;
	if (NULL!=(in=fopen(filename,"r"))) {
		int count = 0, totalsize = 80;
		char* cursor;
		buffer = malloc(totalsize);
		cursor = buffer;
		while (1) {
			char ch = fgetc(in);	
			if (count >= totalsize) {
				buffer = realloc(buffer,(totalsize+=80));
				CHECK(NULL!=buffer);
				cursor = buffer + count;
			}	
			count++;
			if (ch == EOF) {
				*cursor = '\0';
				break;
			} 
			*cursor = ch;
			cursor++;
		}
		fclose(in);
	} 
	return buffer;
}

void loadGPRSConfig (void) {};
void saveGPRSConfig(struct gprsConfig* configs) {};
point getScreenCenter(void) {point p; return p;};
void show_bmp(const char* filename){}
#endif

typedef unsigned char u_char;

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const u_char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\301', '\302', '\303', '\304', '\305', '\306', '\307',
	'\310', '\311', '\312', '\313', '\314', '\315', '\316', '\317',
	'\320', '\321', '\322', '\323', '\324', '\325', '\326', '\327',
	'\330', '\331', '\332', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int
strcasecmp(const char* s1, char* s2)
{
	register const u_char *cm = charmap,
			*us1 = (const u_char *)s1,
			*us2 = (const u_char *)s2;

	while (cm[*us1] == cm[*us2++])
		if (*us1++ == '\0')
			return (0);
	return (cm[*us1] - cm[*--us2]);
}

int
strncasecmp(const char* s1, const char* s2, register n)
{
	if (n != 0) {
		register const u_char *cm = charmap,
				*us1 = (const u_char *)s1,
				*us2 = (const u_char *)s2;

		do {
			if (cm[*us1] != cm[*us2++])
				return (cm[*us1] - cm[*--us2]);
			if (*us1++ == '\0')
				break;
		} while (--n != 0);
	}
	return (0);
}


char *
strcasestr(const char* s, const char* find)
{
	char c, sc;
	size_t len;

	if ((c = *find++) != 0) {
		c = tolower((unsigned char)c);
		len = strlen(find);
		do {
			do {
				if ((sc = *s++) == 0)
					return (NULL);
			} while ((char)tolower((unsigned char)sc) != c);
		} while (strncasecmp(s, find, len) != 0);
		s--;
	}
	return ((char *)s);
}

/*
char *strcasestr(const char *haystack, const char *needle){
     char *cp,*cp1,*cp2;

     cp = (char *)malloc(strlen(haystack)+1);
     cp1 = (char *)malloc(strlen(needle)+1);
     strcpy(cp,haystack);
     strcpy(cp1,needle);
     for (cp2 = cp; *cp2 != 0; cp2++) *cp2=toupper(*cp2);
     for (cp2 = cp1; *cp2 != 0; cp2++) *cp2=toupper(*cp2);
     cp2 = strstr(cp,cp1);
     if (cp2)
         return (char *)(haystack + (cp2 -cp));
     else
         return NULL; 
}
*/

/*
Converts uppercase characters, in the string provided, to lower case
*/
void convertLower(char * buffer,char * convertBuffer,int length)
{
    int index;
    for (index=0;(index < length);index++) {
        if ((buffer[index] >= 'A') && (buffer[index] <= 'Z')) {
            convertBuffer[index] = ((buffer[index] - 'A') + 'a');
        } else {
            convertBuffer[index] = buffer[index];
        }
    }
}

// You must free the result if result is non-NULL.
char *strReplace(const char *orig, const char *rep, const char *with) {
    char *result; // the return string
    char *ins;    // the next insert point
    char *tmp;    // varies
    int len_rep;  // length of rep
    int len_with; // length of with
    int len_front; // distance between rep and end of last rep
    int count;    // number of replacements

    if (!orig)
        return NULL;
    if (!rep || !(len_rep = strlen(rep)))
        return NULL;
    if (!(ins = strcasestr(orig, rep))) 
        return NULL;
    if (!with)
        with = "";
    len_with = strlen(with);

    for (count = 0; tmp = strcasestr(ins, rep); ++count) {
        ins = tmp + len_rep;
    }

    // first time through the loop, all the variable are set correctly
    // from here on,
    //    tmp points to the end of the result string
    //    ins points to the next occurrence of rep in orig
    //    orig points to the remainder of orig after "end of rep"
    tmp = result = malloc(strlen(orig) + (len_with - len_rep) * count + 1);

    if (!result)
        return NULL;

    while (count--) {
        ins = strcasestr(orig, rep);
		CHECK(NULL!=ins);
        len_front = ins - orig;
        tmp = strncpy(tmp, orig, len_front) + len_front;
        tmp = strcpy(tmp, with) + len_with;
        orig += len_front + len_rep; // move to next "end of rep"
    }
    strcpy(tmp, orig);
    return result;
}

static struct markup {
	char* markup;
	char* replacement;
} markuptab[] = {
	"%20", " ",
	"%21", "!",	
	"%22", "\"", 
	"%23", "#",
	"%24", "$",
	"%25", "%",
	"%26", "&",
	"%27", "'",
	"%28", "(",
	"%29", ")",
	"%2a", "*",
	"%2b", "+",
	"%2c", ",",
	"%2d", "-",
	"%2E", ".",
	"%2f", "/",
	"%30", "0",
	"%31", "1",
	"%32", "2",
	"%33", "3",
	"%34", "4",
	"%35", "5",
	"%36", "6",
	"%37", "7",
	"%38", "8",
	"%39", "9",
	"%3a", ":",
	"%3b", ";",
	"%3c", "<", 
	"%3d", "=",
	"%3e", ">",
	"%3f", "„", 
	"%40", "@",
	"%5b", "[",
	"%5c", "\\",
	"%5d", "]",
	"%5e", "^",	
	"%5F", "_",
	"%60", "`",
	"%7B", "{", 
	"%7c", "|", 
	"%7d", "}", 
	"%7e", "~",
	"%80", "`",
	"%81", "Å",	
	"%82", "Ç",
	"%83", "É",
	"%84", "Ñ",
	"%85", "Ö",
	"%86", "Ü",
	"%87", "á",
	"%88", "à",
	"%89", "â",	
	"%8A", "ä",
	"%8B", "ã",	
	"%8C", "å", 
	"%8D", "ç",	
	"%8E", "é",
	"%8F", "è",
	"%90", "ê",	
	"%91", "ë",
	"%92", "í",
	"%93", "ì",	
	"%94", "î",
	"%95", "ï",	
	"%96", "ñ",
	"%97", "ó",
	"%98", "ò",	
	"%99", "ô",
	"%9A", "ö",
	"%9B", "õ",
	"%9C", "ú",	
	"%9D", "ù",
	"%9E", "û",
	"%9F", "ü",
	"%A0", " ",
	"%A1", "°",
	"%A2", "¢",
	"%A3", "£",
	"%A4", "§",
	"%A5", "•",
	"%A6", "¶",
	"%A7", "ß",
	"%A8", "®",
	"%a9", "©",
	"%AA", "™",
	"%AB", "´",
	"%AC", "¨",	
	"%AD", "",
	"%AE","Æ",
	"%AF", "Ø",
	"%B0", "∞",
	"%B1", "±",
	"%B2", "≤",
	"%B3", "≥",
	"%B4", "¥",
	"%B5", "µ",
	"%B6", "∂",
	"%B7", "∑",
	"%B8", "∏",
	"%B9", "π",
	"%BA", "∫",
	"%BB", "ª",
	"%BC", "º",
	"%BD", "Ω",
	"%BE", "æ",
	"%BF", "ø",
	"%C0", "¿",
	"%C1", "¡",
	"%C2", "¬",
	"%C3", "√",
	"%C4", "ƒ",
	"%C5", "≈",
	"%C6", "∆",
	"%C7", "«",
	"%C8", "»",
	"%C9", "…",
	"%CA", " ",
	"%CB", "À",
	"%CC", "Ã",
	"%CD", "Õ",
	"%CE", "Œ",
	"%CF", "œ",
	"%D0", "–",
	"%D1", "—",
	"%D2", "“",
	"%D3", "”",
	"%D4", "‘",
	"%D5", "’",
	"%D6", "÷",
	"%D7", "◊",
	"%D8", "ÿ",
	"%D9", "Ÿ",
	"%DA", "⁄",
	"%DB", "€",
	"%DC", "‹",
	"%DD", "›",
	"%DE", "ﬁ",
	"%DF", "ﬂ",
	"%E0", "‡",
	"%E1", "·",
	"%E2", "‚",
	"%E3", "„",
	"%E4", "‰",
	"%E5", "Â",
	"%E6", "Ê",
	"%E7", "Á",
	"%E8", "Ë",
	"%E9", "È",
	"%EA", "Í",
	"%EB", "Î",
	"%EC", "Ï",
	"%ED", "Ì",
	"%EE", "Ó",
	"%EF", "Ô",
	"%F0", "",
	"%F1", "Ò",
	"%F2", "Ú",
	"%F3", "Û",
	"%F4", "Ù",
	"%F5", "ı",
	"%F6", "ˆ",
	"%F7", "˜",
	"%F8", "¯",
	"%F9", "˘",
	"%FA", "˙",
	"%FB", "˚",
	"%FC", "¸",
	"%FD", "˝",
	"%FE", "˛",
	"%FF", "ˇ",
};

/*
* \param text must be malloc'ed and this function takes ownership.
* \result shall be freed if non-NULL.
*/
char* removeMarkup(char* text)
{
	char* result;
	int numMarkups = sizeof(markuptab)/sizeof(markuptab[0]);
	int i;
	if (!text) 
		return NULL;
	result = text;
	for (i=0; i<numMarkups;i++) {
		char* result2 = strReplace(result,markuptab[i].markup,markuptab[i].replacement);
		if (result2) {
			if (result) free(result);
			result = result2;
		}
	}
	return result;
}

// If text's width is > than width, then text is chopped and '...' added at the end.
// Note that in this case width must be >= 3, to accomodate for the ellipsis.
// In all cases, a copy of text is returned and must be freed by the caller.
const char* chopWithElipsis(const char* text, int width)
{
	const char* elipsis = "...";
	const int l = strlen(elipsis);
	char* result;
	CHECK(NULL!=text);
	result = strdup(text);
	if (strlen(result)>width) {
		CHECK(width>=l);
		strncpy(result+(width-l),elipsis,l);
		result[width] = '\0';
	}
	return result;
}

#ifdef _CUTEST

void test_strReplace(CuTest* tc)
{	
	char* newStr = strReplace("My%20String","%20"," ");
	CuAssert(tc,"Unexpected string",0==strcmp(newStr,"My String"));
	free(newStr);
}

void test_removeMarkup(CuTest* tc)
{
#define CMP_MARKUP(orig,without) {char* oldStr = strdup (orig); \
	char* newStr = removeMarkup(oldStr); \
	CuAssert(tc,"Unexpected string",0==strcmp(newStr,without));\
	free(newStr);}
	CMP_MARKUP("My%20%26%20String","My & String");
	CMP_MARKUP("Test String","Test String");
	CMP_MARKUP("Jon%20%2f%20Jon","Jon / Jon");
	CMP_MARKUP("Jon%20%2F%20Jon","Jon / Jon");
}

void test_chopWithElipsis(CuTest* tc)
{
	char* newtext = chopWithElipsis("hellobaby",3);
	CuAssertTrue(tc,0==strcmp("...",newtext));
	free(newtext);
	newtext = chopWithElipsis("hellobaby",4);
	CuAssertTrue(tc,0==strcmp("h...",newtext));
	free(newtext);
	newtext = chopWithElipsis("hellobaby",5);
	CuAssertTrue(tc,0==strcmp("he...",newtext));
	free(newtext);
	newtext = chopWithElipsis("hellobaby",6);
	CuAssertTrue(tc,0==strcmp("hel...",newtext));
	free(newtext);
	newtext = chopWithElipsis("hellobaby",20);
	CuAssertTrue(tc,0==strcmp("hellobaby",newtext));
	free(newtext);
	newtext = chopWithElipsis("hellobaby",7);
	CuAssertTrue(tc,0==strcmp("hell...",newtext));
	free(newtext);
}

CuSuite* CuGetUtilSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_strReplace);
	SUITE_ADD_TEST(suite, test_removeMarkup);
	SUITE_ADD_TEST(suite, test_chopWithElipsis);
	return suite;
}

#endif
