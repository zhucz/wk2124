#ifndef __WK2124_EPCA1_H_
#define __WK2124_EPCA1_H_

#include "stm32f10x.h"

void HW_WK2124_epca1_Init(void);
void WK2124_epca1_IRQHandler(void);
void WK2124_epca1_Send_String(uint8_t *src, uint16_t len);


uint16_t Wk2124S2_6_SendBuf(uint8_t *sendbuf,uint16_t len);
uint16_t Wk2124S2_6_GetBuf(uint8_t *getbuf);


#endif
