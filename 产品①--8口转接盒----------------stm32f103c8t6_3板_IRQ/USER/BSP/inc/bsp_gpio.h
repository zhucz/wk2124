#ifndef __BSP_GPIO_H_
#define __BSP_GPIO_H_

#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"

#define GREEN_LED_ON()					GPIO_ResetBits(GPIOA, GPIO_Pin_0)
#define GREEN_LED_OFF() 				GPIO_SetBits(GPIOA, GPIO_Pin_0) 

#define YELLOW_LED_ON() 				GPIO_ResetBits(GPIOA, GPIO_Pin_1)
#define YELLOW_LED_OFF()				GPIO_SetBits(GPIOA, GPIO_Pin_1) 

//D2 
#define SP3485_IN_DE_SEND()			GPIO_SetBits(GPIOA, GPIO_Pin_8) 
#define SP3485_IN_DE_RECV()			GPIO_ResetBits(GPIOA, GPIO_Pin_8)
//D3
#define SP3485_01_DE_SEND()			GPIO_SetBits(GPIOA, GPIO_Pin_11)
#define SP3485_01_DE_RECV()			GPIO_ResetBits(GPIOA, GPIO_Pin_11)
//D4
#define SP3485_02_DE_SEND()			GPIO_SetBits(GPIOA, GPIO_Pin_12)
#define SP3485_02_DE_RECV()			GPIO_ResetBits(GPIOA, GPIO_Pin_12)
//D5
#define SP3485_03_DE_SEND()			GPIO_SetBits(GPIOA, GPIO_Pin_15)
#define SP3485_03_DE_RECV()			GPIO_ResetBits(GPIOA, GPIO_Pin_15)
//D6
#define SP3485_04_DE_SEND()			GPIO_SetBits(GPIOB, GPIO_Pin_3)
#define SP3485_04_DE_RECV()			GPIO_ResetBits(GPIOB, GPIO_Pin_3)
//D7
#define SP3485_05_DE_SEND()			GPIO_SetBits(GPIOB, GPIO_Pin_4)
#define SP3485_05_DE_RECV()			GPIO_ResetBits(GPIOB, GPIO_Pin_4)
//D8
#define SP3485_06_DE_SEND()			GPIO_SetBits(GPIOB, GPIO_Pin_5)
#define SP3485_06_DE_RECV()		  GPIO_ResetBits(GPIOB, GPIO_Pin_5)
//D9
#define SP3485_07_DE_SEND()			GPIO_SetBits(GPIOB, GPIO_Pin_6)
#define SP3485_07_DE_RECV()			GPIO_ResetBits(GPIOB, GPIO_Pin_6)
//D10
#define SP3485_08_DE_SEND()			GPIO_SetBits(GPIOB, GPIO_Pin_7)
#define SP3485_08_DE_RECV()			GPIO_ResetBits(GPIOB, GPIO_Pin_7)


void Bsp_Gpio_Init(void);

#endif
