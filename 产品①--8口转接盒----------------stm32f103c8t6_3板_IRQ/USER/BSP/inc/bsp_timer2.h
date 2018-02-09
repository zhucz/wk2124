#ifndef __BSP_TIMER2_H_
#define __BSP_TIMER2_H_

#include "stm32f10x.h"

void Delay_ms(uint32_t nCount);
void Bsp_TIM2_Init(void);

void cpu_disable_irq(uint8_t openwrt);
void cpu_enable_irq(void);
#endif
