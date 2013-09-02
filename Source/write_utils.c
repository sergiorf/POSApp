#include "../Include/write_utils.h"
#include "../Include/Util.h"
#include <string.h>
#include <stdlib.h>

static char delims[] = "\n";

static int get_numlines(const char* multiline)
{
	if (NULL!=multiline) {
		int numlines = 1;
		char* tmp = strdup(multiline);
		const char* line = strtok(tmp,delims);
		while (NULL!=line) {
			line = strtok(NULL,delims);
			if (NULL!=line) {
				numlines++;
			}
		}
		free(tmp);
		return numlines;
	} else {
		return 0;
	}
}

void write_multiline(const char* multiline,int x,int y,int maxlines)
{
	if (NULL!=multiline) {
		int n = 0;
		const char* line;
		char* tmp = strdup(multiline);
		CHECK(NULL!=tmp);
		line = strtok(tmp,delims);
		if (NULL!=line) {
			while(line!=NULL) {
				CHECK(n<maxlines);
				WRITE_AT(line,x,y+n++);
				line = strtok(NULL,delims);
			}
		} else {
			WRITE_AT(tmp,x,y);
		}
		free(tmp);
	}
}

void write_multilineCentered(const char* multiline)
{
	if (NULL!=multiline) {
		const char* line;
		char* tmp = strdup(multiline);
		int y, numlines = get_numlines(multiline);
		point top, bottom;
		CHECK(NULL!=tmp);
		getScreenDims(&top,&bottom);								
		y = (bottom.y-numlines)/2;
		line = strtok(tmp,delims);
		if (NULL!=line) {
			while(line!=NULL) {
				int x = (bottom.x-strlen(line))/2;
				WRITE_AT(line,x,y++);
				line = strtok(NULL,delims);
			}
		} else {
			WRITE_AT(tmp,(bottom.x-strlen(tmp))/2,y);
		}
		free(tmp);
	}
}

#ifdef _CUTEST

void test_multiline(CuTest* tc)
{
	write_multiline("hello this is\n the test of multiline\n centered lets see what happens",5,10,100);
	write_multilineCentered("O limite diário de 10\nimpressões foi atingido\0");
}

CuSuite* CuGetWriteUtilsSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_multiline);
	return suite;
}

#endif





