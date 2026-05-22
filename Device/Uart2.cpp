#include "Uart2.hpp"
#include "stm32f4xx.h"
#include "system.hpp"
#include "Communication.hpp"

bool Write2(uint8_t*data,uint16_t len)
{
	
	DMA1->HIFCR |=(1<<21)|(1<<20)|(1<<19)|(1<<19)|(1<<16); 
	DMA1_Stream6->M0AR = (uint32_t)data;
	DMA1_Stream6->NDTR = len;
	//使能串口发送
	USART2->CR3 |= (1<<7);
	DMA1_Stream6->CR |= (1<<0);
	return true;
}



void Init_Usart2()
{
	
//打开GPIOD USART2 
	RCC->AHB1ENR |= (1<<3);	
	for(volatile int i = 0;i<1000;++i);
	
	set_register(GPIOD->AFR[0], 7, 5*4, 4);   // PD5
	set_register(GPIOD->AFR[0], 7, 6*4, 4);   // PD6
	set_register(GPIOD->MODER, 0b10,5*2,2); //pd6复用
	set_register(GPIOD->MODER, 0b10,6*2,2);//pd7复用
	GPIOD->OTYPER &= ~(1 << 5);  // 推挽输出
	set_register( GPIOD->PUPDR, 0b01, 6*2 , 2 );
	//打开串口时钟
	RCC->APB1ENR |= (1<<17);
	for(volatile int i = 0;i<1000;++i);
	
	
	USART2->CR1 |=(0<<12)|(1<<3)|(1<<2)|(1<<5);//
	USART2->CR2 |=(0b00<<12);//1停止位
	USART2->BRR = USART2_CLK/115200;
	//使能串口
	USART2->CR1 |=(1<<13);
	
	NVIC_SetPriority(USART2_IRQn,7);
	NVIC_EnableIRQ(USART2_IRQn);
	
	
	//打开DMA1
	RCC->AHB1ENR |= (1<<21);
	for(volatile int i = 0;i<1000;++i);
	DMA1_Stream6->CR = 0;
	DMA1_Stream6->PAR = (uint32_t )&USART2->DR;
	DMA1_Stream6->CR |= (0b100<<25)|(0b01<<16)|(0b00<<11)|(0b01<<6)|(0b00<<13)|(1<<10) |(1<<4);
	NVIC_SetPriority(DMA1_Stream6_IRQn,7);
	NVIC_EnableIRQ(DMA1_Stream6_IRQn);

	UartpROT p;
	p.Write = Write2;

 PortRegisterUAST( p,2);
}

extern "C" void USART2_IRQHandler()
{
	//CT
	if(USART2->SR & (1<<6))
	{
		USART2->CR3 &=~(1<<7);
		USART2->CR1 &= ~(1<<6); 
	}
}

extern "C" void DMA1_Stream6_IRQHandler()
{
	if(DMA1->HISR & (1<<21)) 
	{
		DMA1->HIFCR |=(1<<21)|(1<<20)|(1<<19)|(1<<19)|(1<<16);
		USART2->CR1 |= (1<<6);//TC 
	}
}
