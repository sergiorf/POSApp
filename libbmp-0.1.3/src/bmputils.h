#ifndef __bmpfile_utils_h__
#define __bmpfile_utils_h__
#include "bmpfile.h"

typedef enum {
  BMP_OK,
  BMP_ERR_OPEN,
  BMP_ERR_INVALID,
} bmp_ret_t;

bmp_ret_t bmp_readSize(const char* file, uint16_t* width, uint16_t* height);

#endif /* __bmpfile_utils_h__ */
