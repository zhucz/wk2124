/*------------------------------------------------------------------------------
 | Copyright (c) 2014,南京普天通信股份有限公司.
 | All rights reserved.
 |------------------------------------------------------------------------------
 | 文件名称： Silion labs-sim3u146 接口板完整程序
 | 文件批号： 详见任务计划书
 | 简单摘要： 
 |------------------------------------------------------------------------------
 | 当前版本： 1.1
 | 作   者： Mingliang.Lu
 | 完成时间：2014-07-24 
 |------------------------------------------------------------------------------
 | 原版本 ： 1.0
 | 作  者：  MingLiang.lu
 | 完成时间：2013-05-20
 |------------------------------------------------------------------------------
 | 若有问题，可联系: linux.lucian@gmail.com
 |------------------------------------------------------------------------------
 */

#ifndef __MYSERVICE_H__
#define __MYSERVICE_H__

#include <stdbool.h>
#include <stdint.h>

#define V_1		0x02
#define V_2		0x00
#define Y_1   0x02
#define Y_2	  0x00
#define Y_3		0x01
#define Y_4		0x07
#define M_1		0x01
#define M_2   0x01
#define D_1   0x01
#define D_2   0x04
#define T_1   0x08



void mySerial_disable_tx(uint8_t uart_nr);
void mySerial_enable_tx(uint8_t uart_nr);
void mySerial_disable_rx(uint8_t uart_nr);
void mySerial_enable_rx(uint8_t uart_nr);

//使能某个串口进入运行态
void mySerial_send_string(uint8_t uart_nr,uint8_t *src);
//禁止所有串口
void mySerial_disable_all_serial(void);
void reset_mcu_enter_default_mode(void);

#endif //__MYSERVICE_H__

