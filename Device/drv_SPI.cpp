#include "drv_SPI.hpp"
#include "stm32f4xx_hal.h"
#include "system.hpp"


void drv_spiInit()
{
	//打开SPI1时钟
	RCC->APB2ENR |= (1<<12);

	SPI1->CR1 |= (1<<9)|(1<<8)|(0b111<<3)|(1<<2)|(1<<6);

	//打开GPIOA时钟
	RCC->AHB1ENR |= (1<<0);
	//PA5 CLK
	set_register(GPIOA->MODER,0b10,5*2,2);//复用
	set_register(GPIOA->PUPDR,0b01,5*2,2);//上拉
	set_register(GPIOA->OSPEEDR,0b10,5*2,2);//50M
	set_register(GPIOA->AFR[0], 5, 5 * 4, 4);//复用spi1
	
	//PA6 MOSO
	set_register(GPIOA->MODER,0b10,6*2,2);//复用
	set_register(GPIOA->PUPDR,0b01,6*2,2);//上拉
	set_register(GPIOA->OSPEEDR,0b10,6*2,2);//50M
	set_register(GPIOA->AFR[0], 5, 6 * 4, 4);//复用spi1
	

	set_register(GPIOA->MODER,0b10,7*2,2);//复用
	set_register(GPIOA->PUPDR,0b01,7*2,2);//上拉
	set_register(GPIOA->OSPEEDR,0b10,7*2,2);//50M
	set_register(GPIOA->AFR[0], 5, 7 * 4, 4);//复用spi1

}

//读写一个字节
uint8_t spiReadAndWriteByte(uint8_t data)
{
	while ((SPI1->SR & 1 << 1) == 0); 
	SPI1->DR = data;
	while ((SPI1->SR & 1 << 0) == 0);
	return SPI1->DR;
}

//spi速度设置
void spiSetspeed(uint8_t speed)
{
	speed &= 0X07; 
	SPI1->CR1 &= ~(1 << 6);
	SPI1->CR1 &= ~(7 << 3); 
	SPI1->CR1 |= speed << 3; 
	SPI1->CR1 |= 1 << 6; 
}
