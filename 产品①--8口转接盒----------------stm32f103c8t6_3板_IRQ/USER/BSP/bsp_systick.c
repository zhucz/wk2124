#include "bsp_systick.h"
#include "bsp_gpio.h"
#include "core_cm3.h"
#include "misc.h"

volatile uint16_t timer = 0;

volatile uint32_t msTicks;
volatile uint32_t msTicks = 0,bitTimeOut = 0x0;
volatile uint32_t secTicks = 1000,inited = 0;
volatile uint32_t ms_Ticks;
extern void timer_periodic_refresh(void);

/**
  * @brief  void Bsp_Systick_Init(void)
  *         
  * @param  None
  *         
  * @retval None
  */
void Bsp_Systick_Init(void)
{
	SysTick_Config(SystemCoreClock/1000);//1ms 
}

void Run_Led_Flash(void)
{
	static uint8_t i = 0;
	if(i == 0){
		GREEN_LED_ON();
		i = 1;
	}else{
		GREEN_LED_OFF();
		i = 0;
	}
}

void SysTick_Handler(void)  
{
   msTicks++;  
   if(msTicks == 1000){
      inited = 1;
   }

   if(bitTimeOut > 0 ){
      bitTimeOut -= 1;
   }

   if(inited == 1){
      timer_periodic_refresh();
   }

   if(ms_Ticks > 0){
      ms_Ticks -= 1;
   }  
}
