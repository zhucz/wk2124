/**
  ******************************************************************************
  * @file    newlist.c
  * @author  MingLiang.Lu
  * @version V1.1.0
  * @date    16-November-2014
  * @brief   for use to sim3U1xxx devices families.
  * @descriptions: This file is writen based on linkerlist.c MingLiang.Lu,
						 10-october-2014 and feel free to let me know 
						 if you have any questions,you can send e-mail to  
						 linux.lucian@gmail.com or www.potevio.com
  ******************************************************************************
  */

#include "newList.h"

#define		FLASE	0
#define		TRUE	1

/*
 *	@brief :单链表分为 有头节点 和 没有头节点 两种。使用有头节点作为例子，比较直接，容易让人理解。
 */
OS_NEWTASKLIST *pNewHead = NULL; //正常数据用的链表

/*
 *	@brief :step1 --> 链表需要一个头（非必要），用途类似于图书馆书架上的目录书签一样
 *					  				让人ARM处理器容易找到这片内存，尤其在多链表的时候，用途更明显。
 *
 */
 
elemType createNewList( void )
{
	pNewHead = (OS_NEWTASKLIST *)malloc(sizeof(OS_NEWTASKLIST));
	
	if(NULL == pNewHead){
		return FLASE;
	}else{
		Initialize_a_node(pNewHead);
		pNewHead->next = NULL;
		return TRUE;
	}
}

/*
 *	@brief :增加一个节点,实际使用中就是我们的一帧数据
 *
 *	D:\github\vc 6.0\sim3U1xx\linker.c(44) : warning C4715: 'add_a_new_node' : not all control paths return a value
 */

elemType add_a_new_node( elemType *src,elemType type,elemShort len,elemType stat )
{
	unsigned char *pSize = NULL;
	OS_NEWTASKLIST* newNode = NULL;
	
	if(NULL == pNewHead){
		return FLASE;
	}

	
	pSize = (unsigned char *)malloc(len*sizeof(unsigned char));
	if(pSize == NULL){
		return FLASE;
	}
	newNode = (OS_NEWTASKLIST*)malloc(sizeof(OS_NEWTASKLIST));
	if(NULL == newNode){
		return FLASE;
	}else{		
		Initialize_a_node(newNode);	
		newNode->data = pSize;	
		newNode->nPriority = type;			
		newNode->dataLen = len;
		newNode->NodeStat = stat;	
		memcpy(newNode->data,src,len);
		newNode->next = NULL;	
	}
	//有序的往链表中插入节点
	sort_list_from_priority(newNode);

	return TRUE;	
}

/*
 *	@brief : 删除一个节点
 *
 */
elemType  remove_a_old_node( void )
{    	
	 OS_NEWTASKLIST* p = pNewHead->next;
   OS_NEWTASKLIST* q = pNewHead;  
	
   if(NULL == pNewHead){              
      return FLASE;
   }



   while(NULL != p){
      if(DATA_SEND == p->NodeStat){
         free(p->data);
         p->data = NULL;
         if(p->next == NULL){
            q->next = NULL;
         }else{
            q->next = p->next;         //将 被删除的节点 的后一个节点的数据域值赋值给被删除节点的前一个节点的指针域
         }  
         free(p);                      //记得一定要释放到该区域的内存，这样，内存会认为该处可以重新被使用
         p = NULL;                     //这里也很重要，为了严谨起见，释放掉p所指向的标志位后，需要重新置为NULL,以防止乱用	
      }else{
         q = p;
         p = p->next;
      }
   }

   return TRUE;
}


/*
 *	@brief : 初始化一个节点，将里面的成员赋予初始值
 *
 */
void   Initialize_a_node(OS_NEWTASKLIST* node)
{
	node->data = NULL;
	node->dataLen = 0;
	node->NodeStat = 0;	
	node->nPriority = 0;	
}


/*
 *	@brief : 判断一个链表是不是空的，用途在于判别发送链表若没有数据，则直接发送巡检命令
 *
 */
elemType  Is_this_EmptyList( void )
{
	if(NULL == pNewHead->next){
		return TRUE;
	}else{
		return FLASE;
	}
}


/*
 *	@brief : 按照优先级加入新的节点
 *
 */
void sort_list_from_priority(OS_NEWTASKLIST* node)
{
	OS_NEWTASKLIST* p = NULL;
	OS_NEWTASKLIST* q = NULL; 
	
	p = pNewHead->next;
	q = pNewHead;
	
	while( NULL != p){		
		if(p->nPriority > node->nPriority){
			//如果，当前节点的优先级比要加入的低，那么就把当前节点的下一个节点的地址给node,并将node赋予给当前节点
			node->next = q->next;
			q->next = node;
			return;
		}
		q = p;
		p = p->next;		
	}
	//如果当前链中没有比node的优先级低的，那么它只好加在尾部
	q->next = node;
	node->next = NULL;	
}

//------------------------------------------end of file----------------------------------
