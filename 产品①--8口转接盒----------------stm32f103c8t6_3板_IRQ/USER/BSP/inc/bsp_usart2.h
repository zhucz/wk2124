#ifndef __BSP_USART2_H_
#define __BSP_USART2_H_

#include "stm32f10x.h"
#include "stm32f10x_usart.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "stm32f10x_dma.h"

void Bsp_Usart2_Init(void);
void Bsp_Usart2_DMA_Init(void);
uint16_t myUART1_send_multi_bytes(uint8_t *buf , uint16_t len);


void usart2_rx_irq_enable(uint8_t enable);
void open_uart1_rx_function(void);
void usart2_tx_irq_enable(uint8_t enable);


#endif
