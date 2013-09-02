#ifndef __DB_H_
#define	__DB_H_

#include "Util.h"
#include "dict.h"

const char* db_getKey(int type);

ret_code db_getKeys(dict_t* keys);

ret_code db_getContent(int type, char** buf, int* length);

ret_code db_Store(int type, char* buf, int length);

#endif	// __DB_H_

