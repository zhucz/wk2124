#ifndef __WK2124_USART0_H_
#define __WK2124_USART0_H_

#include "stm32f10x.h"

void HW_WK2124_usart0_Init(void);
void WK2124_usart0_IRQHandler(void);
void WK2124_usart0_Send_String(uint8_t *src, uint16_t len);


uint16_t Wk2124S2_8_SendBuf(uint8_t *sendbuf,uint16_t len);
uint16_t Wk2124S2_8_GetBuf(uint8_t *getbuf);


#endif
