#include "../Include/str_utils.h"
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

char* str_strtok(char* str, const char* delims)
{
  static char  * src = NULL;
  char  *  p,  * ret = 0;
  if (str != NULL)
    src = str;
  if (src == NULL)
    return NULL;
  if ((p = strpbrk (src, delims)) != NULL) {
    *p  = 0;
    ret = src;
    src = ++p;
	if (0 == *src) {
		src = NULL;
	}
  } else {
	  ret = src;
	  src = NULL;
  }
  return ret;
}

char* str_toupper(const char* str)
{
	int i = 0;
    int len = strlen(str);
    char* newstr = (char*)malloc(len+1);
    for(i = 0; str[i]; i++)
    {
       newstr[i] = toupper(str[i]);
    }
    newstr[i]= '\0';
	return  newstr;
}

char* str_trim(const char* line)
{
	const char* ptr1 = line, *ptr2;
	char* result;
	if (NULL==line) return NULL;
	while (isspace(*ptr1)) ptr1++;
	if (*ptr1=='\0') return NULL;
	ptr2 = line + strlen(line) - 1;
	while (isspace(*ptr2)) ptr2--;
	ptr2++;
	if (ptr2<ptr1) return NULL;
	result = (char*)calloc((ptr2-ptr1)+1,1);
	memcpy(result,ptr1,(ptr2-ptr1));
	return result;
}

char* str_concat(char* orig, const char* appended)
{
	if (NULL==orig) {
		return (char*)strdup(appended);
	} else {
		size_t len1 = strlen(orig), len2 = strlen(appended);
		char* tmp = (char*)realloc(orig,len1+len2+1);
		if (NULL!=tmp) {
			orig = tmp;
			memcpy(orig+len1,appended,len2);
			orig[len1+len2] = '\0';
			return orig;
		} else {
			// failed to reallocate!
			return tmp;
		}
	}
}

char* str_center(const char* noncentered, int screencols)
{
	size_t len = strlen(noncentered);
	if (len<screencols) {
		char* result = (char*)calloc(screencols+1,1), *ptr = result;
		size_t middle = (screencols-len)/2;
		while (--screencols>=0) *ptr++ = ' ';
		memcpy(result+middle,noncentered,len);
		return result;
	} else {
		return (char*)strdup(noncentered);
	}
}

char* str_trimCenterAndConcat(char* orig, const char* line, int screencols)
{
	char* trimmed = str_trim(line);
	if (NULL!=trimmed) {
		char* centered = str_center(trimmed,screencols);
		orig = str_concat(orig, centered);
		free(centered);
		free(trimmed);
	} else {
		orig = str_concat(orig, line);
	}
	return orig;
}

char* str_multilineCenter(const char* text, int screencols)
{
	char* result = NULL;
	if (NULL!=text) {
		static char delims[] = "\n";
		char* tmp = (char*)strdup(text);
		const char* line;
		int c = strlen(text)-1;
		line = str_strtok(tmp,delims);
		if (NULL!=line) {
			while (line != NULL) {
				if (*line!='\0') {
					result = str_trimCenterAndConcat(result,line,screencols);				
				}
			 	line = str_strtok(NULL,delims);
				if (NULL!=line || text[c]=='\n') {
					result = str_concat(result,"\n");
				}
			}
		} else {
			result = str_trimCenterAndConcat(result,tmp,screencols);
		}
		free(tmp);
	}
	return result;
}

#ifdef _CUTEST

void test_str_toupper(CuTest* tc)
{
	char* tmp = str_toupper("hello");
	CuAssertTrue(tc,0==strcmp("HELLO",tmp));	
	free(tmp);
}

void test_str_multilineCenterHelp(CuTest* tc, const char* text, const char* expected)
{
	char* result = str_multilineCenter(text, 20);
	CuAssertTrue(tc,0==strcmp(expected,result));	
	free(result);
}

void test_str_multilineCenter1(CuTest* tc)
{
	test_str_multilineCenterHelp(tc," sara   \nsara sim     \n","        sara        \n      sara sim      \n");      
	test_str_multilineCenterHelp(tc," sara   \nsara sim     ","        sara        \n      sara sim      ");      
}

void test_str_multilineCenter2(CuTest* tc)
{
	test_str_multilineCenterHelp(tc,"sara   ","        sara        ");      
	test_str_multilineCenterHelp(tc,"sara   \n","        sara        \n");  
}

void test_str_multilineCenter3(CuTest* tc)
{
	test_str_multilineCenterHelp(tc,"sara   ","        sara        ");      
	test_str_multilineCenterHelp(tc,"sara   \n","        sara        \n");      
	test_str_multilineCenterHelp(tc,"sara   \n\n","        sara        \n\n");      
	test_str_multilineCenterHelp(tc,"sara   \n \n","        sara        \n \n");      
	test_str_multilineCenterHelp(tc,"sara   \n\n ","        sara        \n\n ");      
}

void test_str_trim(CuTest* tc)
{
	char* result = str_trim("   bla bla   ");
	CuAssertTrue(tc,0==strcmp(result,"bla bla"));
	free(result);
}

void test_str_concat(CuTest* tc)
{
	char* result = str_concat(NULL,"\n");
	char* orig = (char*)calloc(6,1);
	CuAssertTrue(tc,0==strcmp(result,"\n"));
	free(result);
	memcpy(orig,"hello",5);
	result = str_concat(orig," bob");
	CuAssertTrue(tc,0==strcmp(result,"hello bob"));
	free(result);
}

char* test_str_center(CuTest* tc)
{
	char* result = str_center("blo",10);
	CuAssertTrue(tc,0==strcmp(result,"   blo    "));
	free(result);
	result = str_center("bloblo",2);
	CuAssertTrue(tc,0==strcmp(result,"bloblo"));
	free(result);
}

char* test_str_strtokHelper(CuTest* tc, const char** expected, int numExpected, const char* constData)
{
	int i;
	char* data = strdup(constData);
	char delims[] = ",";
	char* result = str_strtok(data,delims);
	for (i=0;i<numExpected;i++) {
		CuAssertTrue(tc,NULL!=result);
		CuAssertTrue(tc,0==strcmp(expected[i],result));
		result = str_strtok(NULL,delims);
		if (NULL==result) {
			CuAssertTrue(tc,i==numExpected-1);
		}
	}
	free(data);
}

char* test_str_strtok(CuTest* tc)
{
	const char* expected1[] = {"foo","bar","\0","baz","biz","\0"};
	const char* expected2[] = {"foo","bar","\0","baz","biz"};
	const char* expected3[] = {"foo","bar","\0","baz","biz"};
	test_str_strtokHelper(tc,expected1,6,"foo,bar,,baz,biz,,");
	test_str_strtokHelper(tc,expected2,5,"foo,bar,,baz,biz,");
	test_str_strtokHelper(tc,expected2,5,"foo,bar,,baz,biz");
}

CuSuite* CuGetStrUtilsSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_str_toupper);
	SUITE_ADD_TEST(suite, test_str_multilineCenter1);
	SUITE_ADD_TEST(suite, test_str_multilineCenter2);
	SUITE_ADD_TEST(suite, test_str_multilineCenter3);
	SUITE_ADD_TEST(suite, test_str_trim);
	SUITE_ADD_TEST(suite, test_str_concat);
	SUITE_ADD_TEST(suite, test_str_center);
	SUITE_ADD_TEST(suite, test_str_strtok);
	return suite;
}

#endif
