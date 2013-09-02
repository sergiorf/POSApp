#include "bmputils.h"
#include <stdio.h>

/**************************************************************************
 *  fskip                                                                 *
 *     Skips bytes in a file.                                             *
 **************************************************************************/
static void fskip(FILE *fp, int num_bytes)
{
   int i;
   for (i=0; i<num_bytes; i++)
      fgetc(fp);
}

static uint16_t flipOrder(uint16_t val)
{
	return (val&0x000F)<<12 | (val&0x00F0)<<4 | (val&0x0F00)>>4 | (val>>12);
}

/*
	Returns the size of a BMP
*/
bmp_ret_t bmp_readSize(const char* file, uint16_t* width, uint16_t* height)
{
  FILE *fp;

  /* open the file */
  if ((fp = fopen(file,"rb")) == NULL) {
    return BMP_ERR_OPEN;
  }

  /* check to see if it is a valid bitmap file */
  if (fgetc(fp)!='B' || fgetc(fp)!='M')
  {
    fclose(fp);
	return BMP_ERR_INVALID;
  }

  fskip(fp,16);
  fread(width, sizeof(uint16_t), 1, fp);  
  fskip(fp,2);
  fread(height,sizeof(uint16_t), 1, fp);
  
#ifndef _WIN32
  *width = flipOrder(*width);
  *height = flipOrder(*height);
#endif

  fclose(fp);
  return BMP_OK;
}
