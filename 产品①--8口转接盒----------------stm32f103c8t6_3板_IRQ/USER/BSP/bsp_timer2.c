#include "bsp_timer2.h"
#include "stm32f10x_tim.h"

volatile uint16_t timer_flag;


void Bsp_TIM2_Init(void)
{
    TIM_TimeBaseInitTypeDef  TIM2_TimeBaseStructure;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 , ENABLE);

    TIM2_TimeBaseStructure.TIM_Period = 1;
    TIM2_TimeBaseStructure.TIM_Prescaler = (7200 - 1);
    TIM2_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV2;
    TIM2_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Down; 
    TIM_TimeBaseInit(TIM2, &TIM2_TimeBaseStructure);
}


void Delay_ms(uint32_t nCount)
{
  uint32_t TIMCounter = nCount;
  TIM_Cmd(TIM2, ENABLE);
  TIM_SetCounter(TIM2, TIMCounter);
  while(TIMCounter > 1){
    TIMCounter = TIM_GetCounter(TIM2);
  }
  TIM_Cmd(TIM2, DISABLE);
}


void cpu_disable_irq(uint8_t openwrt)
{
//   NVIC_DisableIRQ(UART1_IRQn);
}

void cpu_enable_irq(void)   
{
//   NVIC_EnableIRQ(UART1_IRQn);
}
/*********************************************END OF FILE**********************/
