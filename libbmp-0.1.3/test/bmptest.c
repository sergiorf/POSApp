#include <stdio.h>
#include <bmpfile.h>
#include "bmptest.h"
#include "bmputils.h"

#ifdef _CUTEST

void test_bmpCreate(CuTest* tc)
{
	bmpfile_t* bmp;
	int i,j;
	int width = 64, height = 64, depth = 1; 
	bw_pixel_t pixel;
	pixel.uniColour = 128;
	CuAssertTrue(tc,NULL!=(bmp=bmp_create(64,64,1)));
	for (i = 10, j = 10; j < 64; ++i, ++j) {
		bmp_set_pixel(bmp, i, j, pixel);
		pixel.uniColour++;
		bmp_set_pixel(bmp, i + 1, j, pixel);
		bmp_set_pixel(bmp, i, j + 1, pixel);
	}	
	CuAssertTrue(tc,width==bmp_get_width(bmp));
	CuAssertTrue(tc,height==bmp_get_height(bmp));
	CuAssertTrue(tc,depth==bmp_get_depth(bmp));
	bmp_save(bmp, "testimage.bmp");
	bmp_destroy(bmp);
}

void test_bmpReadSize(CuTest* tc)
{
	uint16_t width, height;
	CuAssertTrue(tc,BMP_OK == bmp_readSize("..\\..\\..\\Resources\\logo_mono.bmp", &width, &height));
	CuAssertTrue(tc, 136==width);
	CuAssertTrue(tc, 136==height);
}

CuSuite* CuGetBmpSuite(void)
{
	CuSuite* suite = CuSuiteNew();
	SUITE_ADD_TEST(suite, test_bmpCreate);
	SUITE_ADD_TEST(suite, test_bmpReadSize);
    return suite;
}

#endif

