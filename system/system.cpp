#include "system.hpp"
#include "stm32f4xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"

#define NVIC_PRIORITYGROUP_4         0x00000003U 
#if 1

	__asm(".global __use_no_semihosting\n\t") ;//注释本行, 方法1
	extern "C"
	{
		struct __FILE {
		int handle;
		};
		std::FILE __stdout;

		void _sys_exit(int x)
		{
			x = x;
		}

		//__use_no_semihosting was requested, but _ttywrch was referenced, 增加如下函数, 方法2
		void _ttywrch(int ch)
		{
			ch = ch;
		}
		
		char *_sys_command_string(char *cmd, int len)
		{
				return 0;
		}
 
	}
#endif

////重写系统滴答函数
extern "C" HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority)
{
	return HAL_OK;
}


static inline void DWT_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk; 
    DWT->CYCCNT = 0;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;           
}


void os_delayUs(uint32_t us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t ticks = us * (SystemCoreClock / 1000000);

    while ((DWT->CYCCNT - start) < ticks);
}






static void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 7;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
		while(1);
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    while(1);
  }
}



void systeamInit(void)
{
	//开启中断分组
	HAL_Init();
	SystemClock_Config();
	HAL_NVIC_SetPriorityGrouping(NVIC_PRIORITYGROUP_4);
}

extern "C" void vPortSetupTimerInterrupt(void)
{
   //打开TIM7全局时基
	RCC->APB1ENR |= (1<<5);
	for( volatile int i = 0 ; i < 1000 ; ++i );
	//分频10M
	TIM7->PSC = (8400-1);
	TIM7->ARR =  (10-1);
	TIM7->CNT = 0;
	//更新中断
	TIM7->DIER |= (1<<0);
	//最高优先级
	NVIC_SetPriority( TIM7_IRQn , 5);
	NVIC_EnableIRQ( TIM7_IRQn );
	//使能定时器
	TIM7->CR1 = (1<<7) | (1<<0);
}

extern "C" void TIM7_IRQHandler()
{
	if(TIM7->SR & (1<<0))
	{ 
    if (xTaskIncrementTick() != pdFALSE)
        portYIELD_FROM_ISR(pdTRUE);
		TIM7->SR &= ~(1<<0);
	}
}

extern "C" void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{

    (void)xTask;
    (void)pcTaskName;
    while(1)
    {
    }
}


void * operator new( std::size_t size )
{
	return pvPortMalloc( size );
}
void * operator new[]( std::size_t size )
{
	return pvPortMalloc(size);
}

void  operator delete(void* __p) _NOEXCEPT
{
	return vPortFree ( __p );
}
void  operator delete[](void* __p) _NOEXCEPT
{
	return vPortFree ( __p );
}

