#include "../Include/db.h"
#include "../Include/dict.h"
#include <dbmgr.h>

static char key[MAX_KEY_LENGTH+1];

const char* db_getKey(int type)
{
	int count, fd;
	char tmpkey[MAX_KEY_LENGTH+sizeof(int)];
	//char buf[128];
	CHECK((type>=0)&&(type<NUM_TYPES));
	fd = open(typetab[type].filename,O_RDONLY|O_CREAT);
	CHECK(fd>=0);
	count = read(fd,tmpkey,MAX_KEY_LENGTH+sizeof(int));
	/*sprintf(buf,"%s-%d-%d-%s",typetab[type].filename,fd,count,key);
	WRITE_AT(buf,0,0);
	SVC_WAIT(2000);*/
	close(fd);
	if (count!=(MAX_KEY_LENGTH+sizeof(int))) {
		memcpy(key,"\0",1); // not found
	} else {
		memcpy(key,tmpkey+sizeof(int),MAX_KEY_LENGTH);
		key[MAX_KEY_LENGTH] = '\0';
	}
	return key;
}

ret_code db_getKeys(dict_t* keys)
{
	int i;
	for (i=0;i<NUM_TYPES;i++) {
		dict_install(keys,typetab[i].type,db_getKey(i));
	}
	return SUCCESS;
}

/**
	Note that the content returned does NOT include the
	4 bytes from the head of the file, which indicate
	the length of the content, in bytes.
	The returned content is NULL terminated.
*/
ret_code db_getContent(int type, char** buf, int* length)
{
	int count, size_content, fd;
	ret_code ret = ERROR;
	CHECK((type>=0)&&(type<NUM_TYPES));
	CHECK((buf!=NULL)&&(length!=NULL));
	fd = open(typetab[type].filename,O_RDONLY|O_CREAT);
	CHECK(fd>=0);
	count = read(fd,(char*)&size_content,sizeof(int));
	if (count==sizeof(int) && (size_content>0)) {
		*length = size_content;
		*buf = (char*)malloc(*length);
		CHECK(*buf!=NULL);
		count = read(fd,*buf,*length);
		ret = (count==*length)? SUCCESS:ERROR;
	}
	close(fd);	
	return ret;
}

ret_code db_Store(int type, char* buf, int length)
{
	int count=0, fd;
	CHECK((type>=0)&&(type<NUM_TYPES));
	fd = open(typetab[type].filename,O_CREAT|O_TRUNC|O_RDWR);
	CHECK(fd>=0);
	if (length>0) {
		char* tmp;
		/*  #bytes      = what?
			sizeof(int) = length (including NULL byte)
			length      = content			
			1           = NULL byte
		*/
		length++; // for the NULL byte
		tmp = (char*)malloc(length+sizeof(int)); 
		CHECK(tmp!=NULL);
		memcpy(tmp,&length,sizeof(int));
		memcpy(tmp+sizeof(int),buf,length-1);
		tmp[sizeof(int)+length-1]='\0';
		count = write(fd,tmp,length+sizeof(int)); 		
		/*sprintf(buf2,"[[[%d-%d]]]",length,count);
		WRITE_AT(buf2,0,0);
		SVC_WAIT(10000);*/		
		if(NULL!=tmp)
			free(tmp);
	}
	close(fd);
	return (count==(length+sizeof(int))? SUCCESS:ERROR);
}

