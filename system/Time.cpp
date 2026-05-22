#include "Time.hpp"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"


uint32_t timeCount = 0;

extern "C" uint32_t HAL_GetTick(void)
{
	return timeCount;
}

void os_delay(uint32_t ms)
{
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
		{
			vTaskDelay(pdMS_TO_TICKS(ms));
		} 
    else
    {
       
       uint32_t t = HAL_GetTick();
       while((HAL_GetTick() - t) < ms);
    }
}


void InitTime()
{
  //打开定时器3
  RCC->APB1ENR |= (1<<1);
  for( volatile int i = 0 ; i < 1000 ; ++i );
	TIM3->PSC = (8400-1);
	TIM3->ARR = (100-1);
	TIM3->CNT = 0;
	TIM3->DIER |= (1<<0);
	NVIC_SetPriority(TIM3_IRQn ,6);
	NVIC_EnableIRQ(TIM3_IRQn );
	TIM3->CR1 = (1<<7) | (1<<0);
}
extern "C" void TIM3_IRQHandler()
{
	if(TIM3->SR & (1<<0))
	{
		TIM3->SR &= ~(1<<0);
		timeCount++;
	}
}

