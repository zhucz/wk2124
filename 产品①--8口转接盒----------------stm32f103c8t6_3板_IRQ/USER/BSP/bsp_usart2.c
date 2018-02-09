#include "bsp_usart2.h"
#include <string.h>
#include "bsp_gpio.h"
#include "bsp_timer2.h"
#include "os_struct.h"
#include "soft_timer.h"

OS_UART1 uart1;
//处理该串口的帧错误等数据
uint16_t uart1_frame_record;
extern struct soft_timer uart1_timer;
extern struct soft_timer led_timer;

void Bsp_Usart2_Init(void)
{
	USART_InitTypeDef	USART2_InitStructure;
	GPIO_InitTypeDef GPIO_InitStructure;
  NVIC_InitTypeDef NVIC_InitStructure; 
	
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_AFIO, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);
	
	
	  /* Enable USART2 Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
  NVIC_Init(&NVIC_InitStructure);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);  //TXD10 --- PA2 --- USART2_TX
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3; 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_Init(GPIOA,&GPIO_InitStructure);  //RXD10 --- PA3 --- USART2_RX
	
	USART2_InitStructure.USART_BaudRate = 115200;
	USART2_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART2_InitStructure.USART_Mode = USART_Mode_Rx|USART_Mode_Tx;
	USART2_InitStructure.USART_Parity =	USART_Parity_No;
	USART2_InitStructure.USART_StopBits =	USART_StopBits_1;
	USART2_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART2,&USART2_InitStructure);
	


  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_0);
  /* Enable the USARTy Interrupt */
  NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;	 
  NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;
  NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  NVIC_Init(&NVIC_InitStructure);

  USART_ITConfig(USART2,USART_IT_RXNE,ENABLE);               //允许串口1接收中断。  
  USART_Cmd(USART2,ENABLE);                                  //启动串口  
  USART_ClearFlag(USART2,USART_FLAG_TC);                     //发送完成标志位 

}

void open_uart1_rx_function(void)
{  
   memset(&uart1.recvBuff[0], 0x0, sizeof(uart1.recvBuff));
   uart1.recvFlag = 0x0;
   uart1.count = 0x0;

   uart1.didex = 0x0;
   uart1.dsize = 0x0;
   uart1.txstat = 0x0;
   uart1.txbuf = NULL;
   
//   SI32_UART_A_enable_rx(SI32_UART_1);
//   SI32_UART_A_enable_tx(SI32_UART_1);
   
   start_timer(&led_timer);
}



//void myUART1_send_multi_bytes(uint8_t *buf , uint16_t len)
//{
//	uart1.txbuf = buf;
//	uart1.dsize = len;
//	uart1.didex = 0;
//	USART2->CR1 |= (1 << 7);//发送缓冲区空中断使能
//	SP3485_IN_DE_SEND();
//	Delay_ms(30); // 30  --- 3ms 
//	
////	while(len--){
////		USART_SendData(USART2, buf[send_index]); 
////		while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
////		send_index++;
////	}
//}



void usart2_rx_irq_enable(uint8_t enable)
{
	if(enable == 1){
//		USART2->SR &= ~(1 << 6);//清除发送完成中断的状态
		USART2->CR1 |= (1 << 2);//使能接收  RE
//		USART2->CR1 |= 	(1 << 5);//RXNEIE:接收缓冲区非空中断使能  当USART_SR中的ORE或者RXNE为‘1’时，产生USART中断  
	}else{
		USART2->CR1 &= ~(1 << 2);//关闭接收  RE
//		USART2->CR1 &= ~(1 << 5);
	}
}


void usart2_tx_irq_enable(uint8_t enable)
{
	if(enable == 1){
		USART2->CR1 |= (1 << 3);//使能发送  TE  
//		USART2->CR1 |= (1 << 6);//使能发送完成中断
	}else{
//		USART2->CR1 &= ~(1 << 6);//关闭发送完成中断
		USART2->CR1 &= ~(1 << 3);//关闭发送  TE 
//		USART2->SR &= ~(1 << 6);	//清除发送完成中断的状态	
	}
}


uint16_t  myUART1_send_multi_bytes(uint8_t *src,uint16_t len)
{
   //代码防御性检查,串口的发送状态检查
   if( uart1.txstat == DEVICE_BUSY){
      return 0;
   }
   
   //使能对上串口的发送485，进入到发送模式
//   SI32_UART_A_disable_rx(SI32_UART_1); 
   usart2_rx_irq_enable(DISABLE);   
   SP3485_IN_DE_SEND();
   
   //将待发送数据的内存地址给uart1的发送缓冲区,并置为发送状态位忙
   uart1.txbuf = src;
   uart1.didex = 0x0;   
   uart1.dsize = len;  
   uart1.txstat = DEVICE_BUSY;
  
   Delay_ms(30);
   
//   //清空发送FIFO，同时使能发送数据的空FIFO数据请求
//   SI32_UART_A_flush_tx_fifo(SI32_UART_1);   
//   SI32_UART_A_enable_tx_data_request_interrupt(SI32_UART_1);    
	 
	USART2->CR1 |= (1 << 7);//发送缓冲区空中断使能
   for(;;){
      if(uart1.txstat == DEVICE_BUSY){
      
      }else{
         break;
      }
   }
   
	return len;
}


void USART2_IRQHandler(void)
{
/*-------串口接收中断-------*/
	if(USART2->SR & (1 << 5)){
		uart1.recvBuff[uart1.count++] = USART2->DR;
		if(uart1.count >= 699){
			 uart1.count=698;     
		}
		
		USART2->SR &= ~(1 << 5);
		
		reload_timer(&uart1_timer,2);
		start_timer(&uart1_timer);    
	}
	
/*-------发送完成中断-------*/
	if(USART2->SR & (1 << 6)){ 
		if(uart1.dsize > uart1.didex) {
			USART2->DR = uart1.txbuf[uart1.didex];
			uart1.didex++;
		}else{
			USART2->CR1 &= ~(1 << 7);//关闭发送缓冲器为空

      uart1.txstat = DEVICE_IDLE;
      uart1.txbuf = NULL;
      uart1.didex = 0x0;   
      uart1.dsize = 0x0;      
      //使能485的接收
//      SI32_UART_A_enable_rx(SI32_UART_1);       
//      rs485_mode_set(0, RS485_RECV);   
	    usart2_rx_irq_enable(ENABLE); 	
	    SP3485_IN_DE_RECV();
			
		}
	}
	
}


//void USART2_IRQHandler(void)
//{
//	/*-------串口接收中断-------*/
//	if(USART_GetFlagStatus(USART2, USART_FLAG_RXNE) != RESET){
//		USART_ClearFlag(USART2, USART_FLAG_RXNE);
//		
//		
//	}
//	
//  /*-------发送完成中断-------*/
//	if(USART_GetFlagStatus(USART2, USART_FLAG_TC) != RESET){
//		 USART_ClearFlag(USART2, USART_FLAG_TC);
//		
//		if(uart1.dsize > uart1.didex){
//			USART_SendData(USART2,uart1.txbuf[uart1.didex]);
//			uart1.didex++;
//		}else{
//			uart1.dsize = 0;
//			uart1.didex = 0;

//		}
//	}
//}

