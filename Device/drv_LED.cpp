#include "drv_LED.hpp"
#include "stm32f4xx.h"
#include "system.hpp"


void LedT()
{
  GPIOE->ODR ^= (1 << 0);
//  os_delay(100);
}

void drv_LED_Init(void)
{
  //打开GPIOE时钟
  RCC->AHB1ENR |= (1 << 4);
  //设置为推挽输出 无上下拉
  //pin0
  set_register(GPIOE->MODER, 0b01, 0 * 2, 2);
  set_register(GPIOE->OSPEEDR, 0b00, 0 * 2, 2);
  set_register(GPIOE->PUPDR, 0b00, 0 * 2, 2);
  //pin1
  set_register(GPIOE->MODER, 0b01, 1 * 2, 2);
  set_register(GPIOE->OSPEEDR, 0b00, 1 *  2, 2);    
  set_register(GPIOE->PUPDR, 0b00, 1 * 2, 2); 

  //pin2
  set_register(GPIOE->MODER, 0b01, 2 * 2, 2);
  set_register(GPIOE->OSPEEDR, 0b00, 2 * 2, 2);
  set_register(GPIOE->PUPDR, 0b00, 2 * 2, 2);
  //pin3
  set_register(GPIOE->MODER, 0b01, 3 * 2, 2);
  set_register(GPIOE->OSPEEDR, 0b10, 3 * 2, 2);
  set_register(GPIOE->PUPDR, 0b00, 3 * 2, 2);
	
}




