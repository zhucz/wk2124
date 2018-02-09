#ifndef __BSP_SPI_H_
#define __BSP_SPI_H_



#include "stm32f10x.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_rcc.h"


#define  Dummy_Byte   0xFF

#define  SPI1_WK2412S1_CS_LOW()      GPIO_ResetBits(GPIOA,GPIO_Pin_4)
#define  SPI1_WK2412S1_CS_HIGH()     GPIO_SetBits(GPIOA,GPIO_Pin_4)

#define  SPI2_WK2412S2_CS_LOW()      GPIO_ResetBits(GPIOB,GPIO_Pin_12)
#define  SPI2_WK2412S2_CS_HIGH()     GPIO_SetBits(GPIOB,GPIO_Pin_12)


#define  SPI1_WK2412S1_NRST_LOW()      GPIO_ResetBits(GPIOB,GPIO_Pin_1)
#define  SPI1_WK2412S1_NRST_HIGH()     GPIO_SetBits(GPIOB,GPIO_Pin_1)

#define  SPI2_WK2412S2_NRST_LOW()      GPIO_ResetBits(GPIOB,GPIO_Pin_11)
#define  SPI2_WK2412S2_NRST_HIGH()     GPIO_SetBits(GPIOB,GPIO_Pin_11)









void Bsp_Spi_Init(void);
uint8_t SPI1_WK2412S1_Read_Write(uint8_t byte);
uint8_t SPI1_WK2412S1_ReadByte(void);
void SPI1_Set_Speed(uint8_t SpeedSet);


uint8_t SPI2_WK2412S2_Read_Write(uint8_t byte);
uint8_t SPI2_WK2412S2_ReadByte(void);
void SPI2_Set_Speed(uint8_t SpeedSet);
#endif
