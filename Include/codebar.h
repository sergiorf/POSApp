#ifndef _CODEBAR_H_
#define _CODEBAR_H_

#define FILENAME_MAX    260
#define BARCODE_C25MATRIX	2
#define BARCODE_C25INTER	3
#define UNICODE_MODE	1
#define ZERROR_TOO_LONG		    5
#define ZERROR_INVALID_DATA	    6
#define ZERROR_INVALID_OPTION	8
#define ZERROR_ENCODING_PROBLEM	9
#define ZERROR_FILE_ACCESS	    10
#define ZERROR_MEMORY		    11
#define SMALL_TEXT		32

struct zint_symbol {
	int symbology;
	int height;
	int whitespace_width;
	int border_width;
	int output_options;
#define ZINT_COLOUR_SIZE 10
	char fgcolour[ZINT_COLOUR_SIZE];
	char bgcolour[ZINT_COLOUR_SIZE];
	char outfile[FILENAME_MAX];
	float scale;
	int option_1;
	int option_2;
	int option_3;
	int show_hrt;
	int input_mode;
#define ZINT_TEXT_SIZE  128
	unsigned char text[ZINT_TEXT_SIZE];
	int rows;
	int width;
#define ZINT_PRIMARY_SIZE  128
	char primary[ZINT_PRIMARY_SIZE];
#define ZINT_ROWS_MAX  178
#define ZINT_COLS_MAX  178
	unsigned char encoded_data[ZINT_ROWS_MAX][ZINT_COLS_MAX];
	int row_height[ZINT_ROWS_MAX]; /* Largest symbol is 177x177 QR Code */
#define ZINT_ERR_SIZE   100
	char errtxt[ZINT_ERR_SIZE];
	char *bitmap;
	int bitmap_width;
	int bitmap_height;
	struct zint_render *rendered;
};

struct zint_symbol *ZBarcode_Create();
void ZBarcode_Delete(struct zint_symbol *symbol);
int ZBarcode_Print(struct zint_symbol *symbol, int rotate_angle);
int ZBarcode_gentofile(const char* code, const char* filename);
int matrix_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length);
int interleaved_two_of_five(struct zint_symbol *symbol, unsigned char source[], int length);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetCodebarSuite(void);
#endif

#endif // _CODEBAR_H_



