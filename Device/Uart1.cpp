#include "Uart1.hpp"
#include "stm32f4xx.h"
#include "system.hpp"
#include "Communication.hpp"

#include "stdio.h"

extern "C" int fputc(int ch, FILE *f)
{
    while(!(USART1->SR & (1<<7))); // TXE
    USART1->DR = ch;
    return ch;
}

#define UART1TIME 1.0*configTICK_RATE_HZ

bool Write1(uint8_t *data,uint16_t len)
{

			DMA2->HIFCR |= (1<<27)|(1<<26)|(1<<25)|(1<<24)|(1<<22);
			DMA2_Stream7->M0AR = (uint32_t)data;
			DMA2_Stream7->NDTR = len;
			//使能
		USART1->CR3 |=(1<<7);//打开DMA发送
		DMA2_Stream7->CR |=(1<<0);

		return true;
}
bool Read1(uint8_t *daTa,uint16_t len)
{

		//清除
		DMA2->LIFCR |=(1<<21)|(1<<20)|(1<<20)|(1<<19)|(1<<18)|(1<<16);
		DMA2_Stream2->M0AR = (uint32_t)daTa;
		DMA2_Stream2->NDTR = len;
		//打开DMA接收
		USART1->CR3 |=(1<<6);
		DMA2_Stream2->CR|=(1<<0);
		return true;
}
extern "C" void DMA2_Stream2_IRQHandler()
{
	if(DMA2->LISR & (1<<21))
	{
		DMA2->LIFCR |=(1<<21)|(1<<20)|(1<<20)|(1<<19)|(1<<18)|(1<<16);
		USART1->CR3 &=~(1<<6);	
	}
}
extern "C" void DMA2_Stream7_IRQHandler()
{
	if(DMA2->HISR & (1<<27))
	{
		DMA2->HIFCR |= (1<<27)|(1<<26)|(1<<25)|(1<<24)|(1<<22);
		USART1->CR1 |= (1<<6);
	}
}

extern "C" void USART1_IRQHandler()
{
	//发送完成
	if(USART1->SR & (1<<6))
	{
		//关闭DMA发送
		USART1->CR3 &=~(1<<7);	
		USART1->CR1 &=~(1<<6);	
	}
}

void SetBuand(uint32_t buand)
{

}

void Init_Usart1()
{
	//打开GPIOA时钟
	RCC->AHB1ENR |= (1<<0);
	for(volatile int i = 0;i<1000;++i);
	//复用功能 TX推挽 RX开漏上拉
	GPIOA->OTYPER |= (1<<10);
	set_register( GPIOA->PUPDR, 0b01, 10*2 , 2 );
	set_register( GPIOA->AFR[1], 7, 9*4-32, 4 );
	set_register( GPIOA->AFR[1], 7, 10*4-32, 4 );
	set_register( GPIOA->MODER, 0b10, 9*2, 2);
	set_register( GPIOA->MODER, 0b10, 10*2, 2);	
	
	//打开时钟
	RCC->APB2ENR |= (1<<4);

	
	USART1->CR1 |= (0<<12)|(1<<3)|(1<<2)|(1<<5);//
	USART1->CR2 |=(0b00<<12);
	USART1->BRR =USART1_CLK/115200;
	USART1->CR1 |= (1<<13);
	
	NVIC_SetPriority(USART1_IRQn,7);
	NVIC_EnableIRQ(USART1_IRQn);
	
	//打开DMA2
	RCC->AHB1ENR |=(1<<22);
	for(volatile int i = 0;i<1000;++i);
	DMA2_Stream6->CR = 0;
	DMA2_Stream7->PAR = (uint32_t)(&USART1->DR);
	DMA2_Stream7->CR |=(0b100<<25)|(0b01<<16)|(0b00<<11)|(0b01<<6)|(0b00<<13)|(1<<10) |(1<<4);
	NVIC_SetPriority(DMA2_Stream7_IRQn,7);
	NVIC_EnableIRQ(DMA2_Stream7_IRQn);
	
	//RX 
	DMA2_Stream2->PAR = (uint32_t)(&USART1->DR);
	DMA2_Stream2->CR |= (0b100<<25)|(0b01<<16)|(0b00<<13)|(0b00<<11)|(1<<10)|(0b00<<6)|(1<<4);
	NVIC_SetPriority(DMA2_Stream2_IRQn,7);
	NVIC_EnableIRQ(DMA2_Stream2_IRQn);
	
	UartpROT p;
	p.Write = Write1;
	p.Read = Read1;
 PortRegisterUAST( p,1);
}


