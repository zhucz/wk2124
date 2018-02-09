/*|-----------------------------------------------------------|
 *|         SPI1:                             WK2124S1:       |
 *| SPI1_NSS   ------- PA4      -------     WK2124S1_SCS      |
 *| SPI1_SCK   ------- PA5      -------     WK2124S1_SCLK     |
 *| SPI1_MISO  ------- PA6      -------     WK2124S1_SDOUT    |
 *| SPI1_MOSI  ------- PA7      -------     WK2124S1_SDIN     |
 *| SPI1_NIRQ  ------- PB0      -------     WK2124S1_NIRQ     |
 *| SPI1_NRST  ------- PB1      -------     WK2124S1_NRST     |
 *------------------------------------------------------------|
 *
 *
 *|-----------------------------------------------------------|
 *|         SPI2:                             WK2124S2:       |
 *| SPI2_NSS   ------- PB12     -------     WK2124S2_SCS      |
 *| SPI2_SCK   ------- PB13     -------     WK2124S2_SCLK     |
 *| SPI2_MISO  ------- PB14     -------     WK2124S2_SDOUT    |
 *| SPI2_MOSI  ------- PB15     -------     WK2124S2_SDIN     |
 *| SPI2_NIRQ  ------- PB10     -------     WK2124S2_NIRQ     |
 *| SPI2_NRST  ------- PB11     -------     WK2124S2_NRST     |
 *------------------------------------------------------------|
 *
 */
#include "bsp_spi.h"
#include "bsp_gpio.h"
#include "stm32f10x_exti.h"
#include "stm32f10x_spi.h"
#include "stm32f10x_gpio.h"
#include "stm32f10x_rcc.h"
#include "misc.h"
#include "bsp_timer2.h"


/*******************************************************************************
* Function Name  : 
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void Bsp_Spi_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	EXTI_InitTypeDef EXTI_InitStructure;
	SPI_InitTypeDef  SPI_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure; 
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOB|RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_SPI1,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_SPI2, ENABLE);
	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable , ENABLE);
	
	//SPI1_GPIO  ------------  WK2124S1
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);  //SPI1_NSS 配置
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5|GPIO_Pin_6|GPIO_Pin_7;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);  //SPI1_SCK,SPI1_MISO,SPI1_MOSI 配置
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);  //SPI1_NRST 配置
	
	//SPI2_GPIO  ------------  WK2124S2
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_12|GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);  //SPI2_NSS,SPI2_NRST 配置
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_13|GPIO_Pin_14|GPIO_Pin_15;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);  //SPI2_SCK,SPI2_MISO,SPI2_MOSI 配置


	//         WK2124S1                              WK2124S2                
	//SPI1_NIRQ -- PB0(EXTI0_IRQHandler)    SPI2_NIRQ -- PB10(EXTI15_10_IRQHandler)  外部中断引脚
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0|GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_Init(GPIOB,&GPIO_InitStructure);  //SPI1_NIRQ,SPI2_NIRQ 配置
	
	/* Enable SPI1_NIRQ -- PB0 Interrupt */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  NVIC_InitStructure.NVIC_IRQChannel = EXTI0_IRQn ;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource0);
	EXTI_InitStructure.EXTI_Line = EXTI_Line0;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStructure);
	
	/* Enable SPI2_NIRQ  ------- PB10 Interrupt */
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
	NVIC_InitStructure.NVIC_IRQChannel = EXTI15_10_IRQn ;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB, GPIO_PinSource10);
	EXTI_InitStructure.EXTI_Line = EXTI_Line10;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
	EXTI_Init(&EXTI_InitStructure);
	

	SPI1_WK2412S1_NRST_HIGH();
	SPI2_WK2412S2_NRST_HIGH();
	
  SPI1_WK2412S1_CS_HIGH();//拉高WK2124S1
  SPI2_WK2412S2_CS_HIGH();//拉高WK2124S2
	
	//SPI1 SPI2 模式配置
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_8b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_Low;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_1Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_8;
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial = 7;
	
	SPI_Init(SPI1,&SPI_InitStructure);
	SPI_Init(SPI2,&SPI_InitStructure);
	SPI_Cmd(SPI1, ENABLE);
	SPI_Cmd(SPI2, ENABLE);

//	SPI1_WK2412S1_Read_Write(0xff);//启动传输		 
//  SPI1_Set_Speed(SPI_BaudRatePrescaler_128);	//设置为10M时钟,高速模式

	SPI1_WK2412S1_NRST_HIGH();//硬件复位WK2124S1
  SPI1_WK2412S1_NRST_LOW();
	Delay_ms(100);
	SPI1_WK2412S1_NRST_HIGH();
	Delay_ms(1000);
	
	SPI2_WK2412S2_NRST_HIGH();//硬件复位WK2124S2
	SPI2_WK2412S2_NRST_LOW();
	Delay_ms(100);
	SPI2_WK2412S2_NRST_HIGH();
	Delay_ms(1000);
}

/*******************************************************************************
* Function Name  : SPI1扩展串口读一个字节
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
uint8_t SPI1_WK2412S1_ReadByte(void)
{
  return (SPI1_WK2412S1_Read_Write(Dummy_Byte));
}

/*******************************************************************************
* Function Name  : SPI1扩展串口发送一个字节
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
uint8_t SPI1_WK2412S1_Read_Write(uint8_t byte)
{
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
  SPI_I2S_SendData(SPI1, byte);
  while (SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_RXNE) == RESET);
  return SPI_I2S_ReceiveData(SPI1);
}

/*******************************************************************************
* Function Name  : 
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void SPI1_Set_Speed(uint8_t SpeedSet)
{
	SPI_InitTypeDef  SPI_InitStructure;
	
	SPI_Cmd(SPI1,DISABLE);
	SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet ;
	SPI_Init(SPI1, &SPI_InitStructure);
	SPI_Cmd(SPI1,ENABLE);
} 








/*******************************************************************************
* Function Name  : SPI2扩展串口读一个字节
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
uint8_t SPI2_WK2412S2_ReadByte(void)
{
  return (SPI2_WK2412S2_Read_Write(Dummy_Byte));
}

/*******************************************************************************
* Function Name  : SPI2扩展串口发送一个字节
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
uint8_t SPI2_WK2412S2_Read_Write(uint8_t byte)
{
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_TXE) == RESET);
  SPI_I2S_SendData(SPI2, byte);
  while (SPI_I2S_GetFlagStatus(SPI2, SPI_I2S_FLAG_RXNE) == RESET);
  return SPI_I2S_ReceiveData(SPI2);
}

/*******************************************************************************
* Function Name  : 
* Description    : 
*  
*
* Input          : None
* Output         : None
* Return         : 
*******************************************************************************/
void SPI2_Set_Speed(uint8_t SpeedSet)
{
	SPI_InitTypeDef  SPI_InitStructure;
	
	SPI_Cmd(SPI2,DISABLE);
	SPI_InitStructure.SPI_BaudRatePrescaler = SpeedSet ;
	SPI_Init(SPI2, &SPI_InitStructure);
	SPI_Cmd(SPI2,ENABLE);
} 

