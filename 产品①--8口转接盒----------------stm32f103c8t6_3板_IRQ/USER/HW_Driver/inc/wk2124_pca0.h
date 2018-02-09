#ifndef __WK2124_PCA0_H_
#define __WK2124_PCA0_H_

#include "stm32f10x.h"

void HW_WK2124_pca0_Init(void);
void WK2124_pca0_IRQHandler(void);
void WK2124_pca0_Send_String(uint8_t *src, uint16_t len);


uint16_t Wk2124S1_4_SendBuf(uint8_t *sendbuf,uint16_t len);
uint16_t Wk2124S1_4_GetBuf(uint8_t *getbuf);


#endif
