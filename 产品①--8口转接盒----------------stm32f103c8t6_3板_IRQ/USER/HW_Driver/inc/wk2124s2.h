#ifndef __WK2124S2_H_
#define __WK2124S2_H_

#include "stm32f10x.h"
#include "bsp_printf.h"

void EXHW_WK2412S2_Init(void);

void EXHW_WK2412S2_Write_Reg(uint8_t reg, uint8_t dat);
uint8_t EXHW_WK2412S2_Read_Reg(uint8_t reg);
void EXHW_WK2412S2_Write_FIFO(uint8_t reg, uint8_t *buf, uint16_t len);
void EXHW_WK2412S2_Read_FIFO(uint8_t reg, uint8_t *buf, uint16_t len);

uint8_t Wk2xxx2Test(void);

void EXHW_WK2412S2_Disable_Tx(uint8_t port);
void EXHW_WK2412S2_Enable_Tx(uint8_t port);
void EXHW_WK2412S2_Disable_Rx(uint8_t port);
void EXHW_WK2412S2_Enable_Rx(uint8_t port);
#endif

