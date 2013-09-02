#include "../Include/valor_utils.h"
#include "../Include/Util.h"
#include <string.h>
#define MAXBUF 128

const char* valor_withCurrency(const char* value)
{
	if (NULL!=value) {
		static char buf[MAXBUF];
	#ifdef WIN32
		_snprintf(buf,MAXBUF,"R$%s\0",value);
	#else
		snprintf(buf,MAXBUF,"R$%s\0",value);	
	#endif	
		return buf;
	} else {
		return NULL;
	}
}

const char* valor_format(const char* value)
{
	if (NULL!=value) {
		static char buf[MAXBUF];
		static char tmp[MAXBUF];
		char* dot;
		int j, n;
	#ifdef WIN32
		_snprintf(buf,MAXBUF,"%s\0",value);
	#else
		snprintf(buf,MAXBUF,"%s\0",value);	
	#endif	
		dot = strchr(buf,'.');
		if (dot) {
			*dot = ',';
			j = (int)(dot-buf);
		} else {
			j = strlen(buf);
		}
		n = (j-1) / 3; // Number of dots to put
		if (n>0) {
			char* dest = tmp, *src = buf;
			int charsleft = strlen(buf), c = j % 3 /* num digits till first dot*/;
			strncpy(dest,src,c);
			dest+=c; src+=c;
			*dest++ = '.';
			while(n-->0) {
				strncpy(dest,src,3);
				if ((charsleft-=3) > 0) {
					dest += 3; src += 3;				
				}
				if (n>0) {
					*dest++ = '.';					
				} else {
					strncpy(dest,src,charsleft);
				}
			}
			return tmp;
		} else {
			return buf;
		}
	} else {
		return NULL;
	}
}

#ifdef _CUTEST

void test_withCurrency(CuTest* tc)
{
	CuAssertTrue(tc,0==strcmp("R$200",valor_withCurrency("200")));	
}

void test_format(CuTest* tc)
{
	CuAssertTrue(tc,0==strcmp("200",valor_format("200")));	
	CuAssertTrue(tc,0==strcmp("200,00",valor_format("200.00")));	
	CuAssertTrue(tc,0==strcmp("2.000,00",valor_format("2000.00")));	
	CuAssertTrue(tc,0==strcmp("22.000,00",valor_format("22000.00")));	
	CuAssertTrue(tc,0==strcmp("2.000.000,00",valor_format("2000000.00")));	
	CuAssertTrue(tc,0==strcmp("2.000.000",valor_format("2000000")));	
	CuAssertTrue(tc,0==strcmp("2.000",valor_format("2000")));	
}

CuSuite* CuGetValorSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_withCurrency);
	SUITE_ADD_TEST(suite, test_format);
	return suite;
}

#endif



