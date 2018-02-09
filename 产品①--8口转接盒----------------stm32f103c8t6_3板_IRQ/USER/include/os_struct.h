/**
  ******************************************************************************
  * @file    OS_STRUCT.h
  * @author  MingLiang.Lu
  * @version V1.1.0
  * @date    09-October-2014
  * @brief   used for sim3U1xxx devices families.
  * @descriptions: feel free to let me know if you have any questions,you can 
						 send e-mail to  linux.lucian@gmail.com
  ******************************************************************************
  */

#ifndef __OS_STRUCT_H
#define __OS_STRUCT_H

#include "newList.h"
#include <stdint.h>

#define     TRAY_ONLINE       0x11
#define     TRAY_OFFLINE      0x22
#define     DEVICE_BUSY       0x66
#define     DEVICE_IDLE       0x33

#define     ILLEGAL_ACCESS    (1<<1)
#define     TRAY_LOST         (1<<0) 
#define     TIMES_20            7 
#define     TIMES_3              3
#define     TRAY_TIMES_3         2

typedef struct os_commonUARTs {
   uint8_t recvBuff[610];//610
   volatile	uint8_t recvFlag;
   volatile	uint8_t sendFlag;
   volatile  uint16_t count;	
}OS_UART;

typedef struct os_uploadUart {
   volatile uint16_t dsize;//send data size
   volatile uint16_t didex;//send data index 
   
   volatile uint16_t count;//recv data index     
   uint8_t  recvBuff[690]; //700
   volatile uint8_t  recvFlag;

   volatile uint8_t  txstat;
   uint8_t* txbuf;
   
}OS_UART1;

typedef struct os_portLost {	
   uint8_t portstat[8];	
   uint8_t illegalaccess[8];
   uint8_t illegalaccess_recover[8];
   uint8_t traylost[8];
   uint8_t traylost_recover[8];   
volatile 	uint8_t portFlag;
}OS_PORTLOST;

typedef struct os_boardinfo {
    volatile   uint16_t soft_version;
    volatile   uint16_t hard_version;	
    volatile   uint8_t  id;	
}OS_BOARDINFO;

typedef struct os_alarm {
   volatile uint16_t idex;
   volatile uint16_t pendingBytes;
   volatile uint8_t flag;
      uint8_t data[100];
}OS_ALARMINFO;

typedef struct os_reSources{
   volatile	uint16_t idex;	
   volatile	uint16_t reSourceNow;
   volatile 	uint8_t  flag;	
   volatile	uint8_t type;	
   uint8_t rxbuf[5000];
   uint8_t tray_units[10];
}OS_RESOURCE;


#endif //__OS_STRUCT_H
