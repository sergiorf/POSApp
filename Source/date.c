#include "../Include/date.h"
#include "../Include/Util.h"
#include <stdio.h>

char* date_makeCopy(const date_t* date)
{
#define MAXLENGTH 100
	char buf[MAXLENGTH];
	CHECK(NULL!=date);
	memset (buf,'\0',sizeof(buf));
	sprintf(buf,"%02d/%02d/%04d",date->dd,date->mm,date->yyyyy);
	return strdup(buf);
}

date_t* date_new(const char* buf)
{
	date_t* ret = NULL;
	if (NULL!=buf && 8==strlen(buf)) {
		int dd,mm,aaaa;
		if (EOF != sscanf(buf,"%2d%2d%4d",&dd,&mm,&aaaa)) {
			if (dd>=1&&dd<=31&&
				mm>=1&&mm<=12&&
				aaaa>0) {
				ret = (date_t*)calloc(1,sizeof(date_t));
				ret->dd = dd;
				ret->mm = mm;
				ret->yyyyy = aaaa;
			}
		}
	}
	return ret;
}

int date_check2(const date_t* date1,const date_t* date2)
{
	if (NULL==date1||NULL==date2) {
		return 0;
	} else {
		if (date1->yyyyy>date2->yyyyy) {
			return 0;
		} else if (date1->yyyyy<date2->yyyyy) {
			return 1;
		} else {
			if (date1->mm>date2->mm) {
				return 0;
			} else if (date1->mm<date2->mm) {
				return 1;
			} else {
				return (date1->dd>date2->dd)? 0 : 1;
			}
		}
	}
}

void date_delete(date_t* date)
{
	if (NULL!=date)
		free(date);
}

#ifdef _CUTEST

static void test_dates(CuTest* tc, const char* d1, const char* d2)
{
	date_t* date1 = date_new(d1);
	date_t* date2 = date_new(d2);
	CuAssertTrue(tc,date_check2(date1,date2));
	CuAssertTrue(tc,!date_check2(date2,date1));
	date_delete(date1);
	date_delete(date2);
}

static void test_date(CuTest* tc, const char* d, int dd, int mm, int yyyy)
{
	date_t* date = date_new(d);
	CuAssertTrue(tc,NULL!=date);
	CuAssertTrue(tc,dd==date->dd);
	CuAssertTrue(tc,mm==date->mm);
	CuAssertTrue(tc,yyyy==date->yyyyy);
	date_delete(date);
}

static void test_date_new(CuTest* tc) 
{
	date_t* date;
	date = date_new("12");
	CuAssertTrue(tc,NULL==date);
	date = date_new("aaaaaaaa");
	CuAssertTrue(tc,NULL==date);
	date = date_new("32112001");
	CuAssertTrue(tc,NULL==date);
	date = date_new("31132001");
	CuAssertTrue(tc,NULL==date);
	test_date(tc,"12112001",12,11,2001);
	test_date(tc,"01022013",1,2,2013);	
}

static void test_date_check2(CuTest* tc) 
{
	test_dates(tc,"30102001","31102001");
	test_dates(tc,"31092001","31102001");
	test_dates(tc,"31102000","31102001");	
}

static void test_date_makeCopy(CuTest* tc) 
{	
	date_t* date = date_new("31102001");
	char* dateStr = date_makeCopy(date);
	CuAssertTrue(tc,NULL!=dateStr);
	CuAssertTrue(tc,0==strcmp(dateStr,"31/10/2001"));
	free(dateStr);
}

CuSuite* CuGetDateSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_date_new);
	SUITE_ADD_TEST(suite, test_date_check2);
	SUITE_ADD_TEST(suite, test_date_makeCopy);
	return suite;
}

#endif

