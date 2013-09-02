#ifndef __MODELO_H_
#define	__MODELO_H_

#include "Util.h"
#include "ingresso.h"

typedef enum {TEXT, IMAGE, BARCODE} reporttype_t;
typedef enum {LEFT, CENTER, RIGHT, NO} alignment_t;
typedef enum {I2OF5, UNKNOWN} barcodetype_t;

typedef struct {
	alignment_t alignment;
	int fontsize;
	char* content;
	reporttype_t report_type;
	int barcode_width;
	int barcode_heigth;
	barcodetype_t barcode_type;
	char* barcode_value;
	char* image_id;
} report_t;

typedef struct {
	int num_reports;
	report_t** reports;
} modelo_t;

// pdata will be NULL terminated
ret_code modelo_download(const ingresso_t* ingresso, char** pdata);
ret_code modelo_download2(const ingresso_t* ingresso, modelo_t** modelo);
// data will be freed
ret_code modelo_parse(char* data, modelo_t** modelo);
void modelo_delete(modelo_t* modelo);

#ifdef _CUTEST
#include "CuTest.h"
CuSuite* CuGetModeloSuite(void);
#endif

#endif	// __MODELO_H_

