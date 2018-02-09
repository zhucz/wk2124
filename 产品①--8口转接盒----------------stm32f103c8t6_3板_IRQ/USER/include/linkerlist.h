/**
  ******************************************************************************
  * @file    linklist.h
  * @author  MingLiang.Lu
  * @version V1.1.0
  * @date    02-October-2014
  * @brief   used for sim3U1xxx devices families.
  * @descriptions: feel free to let me know if you have any questions,you can 
									 send e-mail to  linux.lucian@gmail.com
  ******************************************************************************
  */

#ifndef __LINKER_H
#define __LINKER_H

#define EMPTY				 1

#define DATA_RESOURCE    1
#define DATA_ALARM       2
#define DATA_NORMAL      3
#define DATA_SEND	       4
#define DATA_RECV	   	 5
 
typedef unsigned char  elemType;
typedef unsigned short elemShort;

typedef struct Node{
	elemType data[380];
	elemType dataType;
	elemShort dataLen;
	elemType NodeStat;
	struct Node *next;
}OS_TASKLIST;

elemType createList( void );
elemType add_a_Node( elemType *src,elemType type,elemShort len,elemType stat );
elemType remove_a_Node( void );
elemType Is_EmptyList( void );


#endif //__LINKER_H

//-----------------------------------------------eof------------------------------
