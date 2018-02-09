/**
  ******************************************************************************
  * @file    :soft_timer.c used for Potevio interface board
  * @author  :linux.lucian@gmail.com
  * @version :Version-1.0
  * @date    :2014-08-23
  * @brief   :source file for software timer.
  * 
  ******************************************************************************
  */

#ifndef __CF8051_H__
#define __CF8051_H__

#include "stdint.h"
//#include "gModes.h"
#include "stm32f10x.h"
//#include "sim3u1xx.h"
//#include <si32_device.h>
//#include <SI32_TIMER_A_Type.h>
//#include "myTIMER1.h"
//#include "myCPU.h"
#include <stdio.h>
#include <string.h>

#include "crc8.h"
#include "myService.h"
#include "os_struct.h"
#include "newList.h"
#include "timerOut.h"
#include "soft_timer.h"
#include "special_buff.h"
// application
//#include "myFLASHCTRL0.h"

#define BACK_TO_NORMAL              (0x0)
#define PORT_RUNNING_NORMAL         (0x0)
#define PORT_LOST_NOWING            (0x4)
#define TRAY_ILLEAGLE_INPUT_NOWING  (0x2)

// extern OS_USART0 usart0;
// extern OS_UART usart1,epca0,pca0,pca1,uart0,uart1;

extern OS_UART    uart0, usart1,pca1, pca0, epca0, epca1, epca2,usart0;
extern OS_UART1	uart1;



extern struct soft_timer  update_startup_timer,update_restart_timer,update_write_packets_timer;

extern uint8_t frame_is_right[7];
extern uint8_t frame_is_wrong[7];
extern volatile uint8_t serivce_time_out;
extern uint8_t uCom_send_dataBase[8][COM_TX_SIZE];
extern struct myflash DeviceInfo;
extern uint8_t TrayInfo[17]; 

//单元板软件升级部分
extern uint8_t cf8051update[40];
extern uint8_t update_trays[8];
extern uint8_t update_trays_packet_status[8];
extern uint8_t update_trays_startup_status[8];
extern uint8_t update_trays_restart_status[8];

void cf8051_service_routine(void);
uint8_t back_trays_types(uint8_t dat);
#endif /*__CF8051_H__*/

