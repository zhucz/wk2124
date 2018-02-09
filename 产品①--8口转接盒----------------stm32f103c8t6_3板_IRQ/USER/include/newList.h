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
#ifndef __NEWLINKER_H
#define __NEWLINKER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define EMPTY				 	   1

#define DATA_RESOURCE    1
#define DATA_ALARM       2
#define DATA_NORMAL      3
#define DATA_SEND	       4
#define DATA_RECV	   	 	 5
 
typedef unsigned char     elemType;
typedef unsigned char *   dataType; 
typedef unsigned short    elemShort;

typedef struct newNode{
   dataType    data;  			//该节点的数据区指针
	elemType    nPriority;     //该节点的优先级
	elemShort   dataLen;       //该节点的数据域长度
	elemType    NodeStat;      //节点的数据的状态 
	//下一个节点
	struct newNode *next;         
}OS_NEWTASKLIST;

elemType  createNewList( void );
elemType  add_a_new_node( elemType *src,elemType type,elemShort len,elemType stat );
elemType  remove_a_old_node( void );
elemType  Is_this_EmptyList( void );
void      Initialize_a_node(OS_NEWTASKLIST* node);
void sort_list_from_priority(OS_NEWTASKLIST* node);

#endif //__NEWLINKER_H

//-----------------------------------------------eof------------------------------
