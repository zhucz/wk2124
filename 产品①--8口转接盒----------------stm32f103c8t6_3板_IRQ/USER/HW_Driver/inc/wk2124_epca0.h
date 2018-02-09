#ifndef __WK2124_EPCA0_H_
#define __WK2124_EPCA0_H_

#include "stm32f10x.h"

void HW_WK2124_epca0_Init(void);
void WK2124_epca0_IRQHandler(void);
void WK2124_epca0_Send_String(uint8_t *src, uint16_t len);


uint16_t Wk2124S2_7_SendBuf(uint8_t *sendbuf,uint16_t len);
uint16_t Wk2124S2_7_GetBuf(uint8_t *getbuf);


#endif
