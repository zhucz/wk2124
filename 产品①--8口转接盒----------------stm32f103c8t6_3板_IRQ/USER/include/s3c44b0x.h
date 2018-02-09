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

#ifndef __MAINBOARD_H__
#define __MAINBOARD_H__

//#include <SI32_RSTSRC_A_Type.h>
#include "stm32f10x.h"
#include <string.h>
#include <stdint.h>
//#include "myCPU.h"
//#include "myTIMER0.h"//相当于以前的usart

#include "crc8.h"
#include "os_struct.h"
#include "newList.h"
#include "soft_timer.h"
#include "special_buff.h"
//#include "myFLASHCTRL0.h"

#define RIGHT      1
#define WRONG      0

//#define BACK_TO_NORMAL              (0x0)
//#define PORT_RUNNING_NORMAL         (0x0)
//#define PORT_LOST_NOWING            (0x4)
//#define TRAY_ILLEAGLE_INPUT_NOWING  (0x2)

extern OS_BOARDINFO     board;
extern OS_PORTLOST      ports;
extern OS_UART          time0;
extern OS_UART1					uart1;

extern OS_NEWTASKLIST   *pNewHead;
extern OS_ALARMINFO     alarm;
extern OS_RESOURCE      reSource;
extern struct soft_timer resource_timer;
extern struct soft_timer  update_startup_timer,update_restart_timer,update_write_packets_timer;
extern struct soft_timer  yijian_read_version_timer;	
extern uint8_t frame_is_right[7];
extern uint8_t frame_is_wrong[7];
extern uint8_t frame_run_ok[11];
extern uint8_t uCom_send_dataBase[8][COM_TX_SIZE];
extern struct myflash DeviceInfo;

uint8_t mainboard_service_routine(void);

#endif //__MAINBOARD_H__

