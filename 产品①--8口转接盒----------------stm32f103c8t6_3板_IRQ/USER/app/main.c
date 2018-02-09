#include "stm32f10x.h"
#include "bsp_gpio.h"
#include "bsp_systick.h"
#include "bsp_usart2.h"
#include "bsp_timer2.h"
#include "timerOut.h"
#include "os_struct.h"
#include "soft_timer.h"  
#include "bsp_printf.h"
#include "bsp_spi.h"
#include "wk2124s.h"
#include "wk2124s2.h"
#include "wk2124_uart0.h"
#include "wk2124_usart1.h"
#include "wk2124_pca1.h"
#include "wk2124_pca0.h"
#include "wk2124_epca2.h"
#include "wk2124_epca1.h"
#include "wk2124_epca0.h"
#include "wk2124_usart0.h"

#include "crc8.h"
#include "cf8051.h"
#include "s3c44b0x.h"
#include "os_struct.h"
#include "newList.h"
#include "timerOut.h"
#include "board.h"

extern struct soft_timer led_timer;
extern OS_UART1 uart1;
extern volatile uint8_t kaiji_cmd;

static void Bsp_Board_Init(void)
{
	Bsp_Gpio_Init();
	Bsp_Systick_Init();
	Bsp_Usart2_Init();
	Bsp_TIM2_Init();
	Bsp_Spi_Init();
	
	EXHW_WK2412S_Init();
	EXHW_WK2412S2_Init();
	
	
	HW_WK2124_uart0_Init();
	HW_WK2124_usart1_Init();
	HW_WK2124_pca1_Init();
	HW_WK2124_pca0_Init();
	HW_WK2124_epca2_Init();
	HW_WK2124_epca1_Init();
	HW_WK2124_epca0_Init();
	HW_WK2124_usart0_Init();
	Bsp_Printf_Init();
}


int main(void)
{
	SystemInit();
  __set_PRIMASK(1);//关闭总中断
	Bsp_Board_Init();
	
	active_myPherial_from_reset();	

	SP3485_IN_DE_RECV();
	
	SP3485_01_DE_RECV();
	SP3485_02_DE_RECV();
	SP3485_03_DE_RECV();
	SP3485_04_DE_RECV();
	
	SP3485_05_DE_RECV();
	SP3485_06_DE_RECV();
	SP3485_07_DE_RECV();
	SP3485_08_DE_RECV();
	open_uart1_rx_function();
	Delay_ms(1000);
  __set_PRIMASK(0);//开启总中断
	printf("odf 8 + 1 \r\n");

#if 1
   while (1)
   {
      if(uart1.recvFlag){
         //主控来了请求数据，146便进入业务处理模式
         mainboard_service_routine();
      }else{
         //主控没请求数据，那就进入到正常的对单元板的巡检模式
				if(kaiji_cmd == 1){
					cf8051_service_routine();
				}
      }        
   }
#else
	while(1){
		Delay_ms(20000);
//		WK2124_uart0_Send_String(tmp_dat, 9);
//		Wk2xxxSendBuf(tmp_dat,512);
//		WK2124_usart1_Send_String(tmp_dat, 9);
//		WK2124_pca0_Send_String(tmp_dat, 9);
//		WK2124_epca2_Send_String(tmp_dat, 9);
//		WK2124_epca1_Send_String(tmp_dat, 9);
//		WK2124_epca0_Send_String(tmp_dat, 9);
//		WK2124_usart0_Send_String(tmp_dat, 9);
//		myUART1_send_multi_bytes(tmp_dat2,8);
	}
#endif
}
