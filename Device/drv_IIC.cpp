#include "drv_IIC.hpp"
#include "system.hpp"
#include "stm32f4xx.h"
#include "FreeRTOS.h"
#include "event_groups.h"
#include "semphr.h"
#include "queue.h"



#define IIC_START (1<<8)
#define IIC_ADDR7(x) (x<<1)
#define IIC_STOP (1<<9)

//状态
#define IIC_SB (1<<0)
#define IIC_ADDR (1<<1)
#define IIC_TxE (1<<7)
#define IIC_RxE (1<<6)
#define IIC_BTF (1<<2)
#define SEND  (1<<0)


#define IIC_TIME  1.0*configTICK_RATE_HZ
#define RxStreamBufferSize 1024
#define RxBUFSIZE 512

//接收缓冲
static EventGroupHandle_t iic_event = xEventGroupCreate();

//IIC 互斥锁
static SemaphoreHandle_t iicMutex = xSemaphoreCreateRecursiveMutex();

uint8_t FRaed = 0;
uint8_t BTFP = 0;
bool iicReadDataFlag = false;
bool iicTxDataFlag = false;


struct IIC_DATA
{
	uint8_t addr;
	uint8_t reg;
	uint8_t *senddata;
};
IIC_DATA iic;
IIC_DATA Txiic;


//传输长度
uint16_t RxdataLen ;
uint16_t TxdataLen ;
uint16_t StreamRxLen;


//接收数据
bool IIC_ReceiveAdder7(uint8_t addr,uint8_t reg,uint8_t*data,uint16_t len,float TIME_OUT)
{
	
	uint32_t waitTicks;
	if(TIME_OUT >= 0)
		waitTicks = TIME_OUT*configTICK_RATE_HZ;
	else
		waitTicks = portMAX_DELAY;
	if(xSemaphoreTakeRecursive(iicMutex,waitTicks)== pdTRUE )
	{
		I2C1->CR1 |= (1<<0);
		//清空
		DMA1->HIFCR |= (1<<11)|(1<<10)|(1<<9)|(1<<8)|(1<<6);
		xEventGroupClearBits(iic_event,IIC_BTF);
		DMA1_Stream5->M0AR = (uint32_t)data;
		DMA1_Stream5->NDTR = len;
		I2C1->CR2 |= (1<<12);
		iic.addr = addr;
		iic.reg = reg;
		RxdataLen = len;
		StreamRxLen = len;
		iicReadDataFlag = true;
		//启动IIC 
		I2C1->CR1 |= IIC_START;
		//等待传输完成
		xEventGroupWaitBits(iic_event,IIC_BTF,pdTRUE,pdFALSE,IIC_TIME);
		xSemaphoreGiveRecursive(iicMutex);
		I2C1->CR1 &=~(1<<0);
		return true;
	}
	else
		return false;
	
}

bool IIC_SendData(uint8_t addr,uint8_t reg,uint8_t*data,uint16_t len, float TIME_OUT)
{
	uint32_t waitTicks;
	if(TIME_OUT >= 0)
		waitTicks = TIME_OUT*configTICK_RATE_HZ;
	else
		waitTicks = portMAX_DELAY;
	if(xSemaphoreTakeRecursive(iicMutex,waitTicks)== pdTRUE )
	{
		 
		I2C1->CR1 |= (1<<0);
		//清空
		xEventGroupClearBits(iic_event,SEND);
		Txiic.addr = addr;
		Txiic.reg = reg;
		Txiic.senddata = data;
		TxdataLen = len;
		iicTxDataFlag = true;
		
		I2C1->CR1 |= IIC_START;
		xEventGroupWaitBits(iic_event,SEND,pdTRUE,pdFALSE,IIC_TIME);
		xSemaphoreGiveRecursive(iicMutex);
		I2C1->CR1 &=~(1<<0);
		return true;
	}
	else
		return false;
}

extern "C" void I2C1_EV_IRQHandler(void)
{
	BaseType_t HigherPriorityTaskWoken = pdFALSE;
	uint32_t tmp = I2C1->SR1;
	
	if(tmp & IIC_SB)
	{
		if(FRaed & (1<<1))
		{
			I2C1->DR = IIC_ADDR7(iic.addr) |1;	
			FRaed &= ~(1<<1);
			FRaed |= (1<<2);

		}
		else
		{
			if(iicReadDataFlag)
				I2C1->DR = IIC_ADDR7(iic.addr) |0;	
			else
				I2C1->DR = IIC_ADDR7(Txiic.addr) |0;	
		}
			
	}
	if(tmp & IIC_ADDR)
	{
		if(FRaed & (1<<2))
		{
			if(RxdataLen == 1) //只接收一个字节
			{
				uint32_t i;
				i= I2C1->SR1;
				i = I2C1->SR2;
				FRaed &= ~(1<<2);
				I2C1->CR1 &= ~(1<<10);
				(void)i;
				I2C1->CR1 |= IIC_STOP;
			}
			else
			{//接收多个字节
				
				uint32_t i;
				i= I2C1->SR1;
				i = I2C1->SR2;
				(void)i;
				I2C1->CR1 |= (1<<10);
				I2C1->CR2|=(1<<11);
				DMA1_Stream5->CR |=(1<<0);
			}
			
		}
		else
		{ 	
			uint32_t i = I2C1->SR1;
			i = I2C1->SR2;
			if(iicTxDataFlag)
			I2C1->DR = Txiic.reg;
			//读模式
			if(iicReadDataFlag)
			{
				I2C1->DR = iic.reg;
				FRaed |= (1<<0);
			}
		}
		
	}
	 if(tmp & IIC_TxE)
	{
		//地址已发送 发送数据
		if(iicTxDataFlag)
		{
			if(TxdataLen >0)
			{
				I2C1->DR = *Txiic.senddata++;
				TxdataLen--;
			}
		}
	}
	if(tmp&IIC_BTF)
	{
		if(FRaed & (1<<0))//重复起始位
		{
			I2C1->CR1 |= IIC_START;
			FRaed &=~ (1<<0);
			FRaed |= (1<<1);
		}
		if(iicTxDataFlag && TxdataLen==0)
		{
			I2C1->CR1 |= IIC_STOP;
			iicTxDataFlag = false;
			xEventGroupSetBitsFromISR(iic_event,SEND,&HigherPriorityTaskWoken);
		}
	}
	portYIELD_FROM_ISR(HigherPriorityTaskWoken);
} 

extern "C" void DMA1_Stream5_IRQHandler()
{
	BaseType_t HigherPriorityTaskWoken = pdFALSE;
	if(DMA1->HISR & (1<<11)) 
	{
		DMA1->HIFCR |= (1<<11)|(1<<10)|(1<<9)|(1<<8)|(1<<6);
		I2C1->CR1 |= IIC_STOP;
		xEventGroupSetBitsFromISR(iic_event,IIC_BTF,&HigherPriorityTaskWoken);
    iicReadDataFlag = false;
    FRaed = 0;
	}
	portYIELD_FROM_ISR(HigherPriorityTaskWoken);
}

void IIC_Init()
{
    // 使能 GPIOB 时钟
    RCC->AHB1ENR |= (1 << 1);
    for( volatile int i = 0 ; i < 1000 ; ++i );

    // 引脚开漏 PB6 SCL, PB7 SDA
    set_register(GPIOB->OTYPER, 1, 6 * 1, 1); // PB6
    set_register(GPIOB->OTYPER, 1, 7 * 1, 1); // PB7
    // 中速
    set_register(GPIOB->OSPEEDR, 0b10, 6 * 2, 2); // PB6
    set_register(GPIOB->OSPEEDR, 0b10, 7 * 2, 2); // PB7
    // 复用 I2C1
    set_register(GPIOB->AFR[0], 4, 6 * 4, 4); // PB6
    set_register(GPIOB->AFR[0], 4, 7 * 4, 4); // PB7

    // 模式：复用功能
    set_register(GPIOB->MODER, 0b10, 6 * 2, 2); // PB6
    set_register(GPIOB->MODER, 0b10, 7 * 2, 2); // PB7

    // 使能 I2C1 时钟
    RCC->APB1ENR |= (1 << 21);
		for( volatile int i = 0 ; i < 1000 ; ++i );
		//复位IIC
    I2C1->CR1 &= ~(1 << 0);
    I2C1->CR2 = 42;
    I2C1->CCR = 35 | (1 << 15);
    I2C1->TRISE = 43;
    //开启中断
		I2C1->CR2 |=(1<<8)|(1<<9)|(1<<10);
    I2C1->CR1 |= (1 << 10); // ACK
		

		NVIC_SetPriority(I2C1_EV_IRQn,6);
		NVIC_EnableIRQ(I2C1_EV_IRQn);
		NVIC_SetPriority(I2C1_ER_IRQn,6);
		NVIC_EnableIRQ(I2C1_ER_IRQn);
		//接收

		//开启DMA1
		RCC->AHB1ENR |= (1<<21);
		for( volatile int i = 0 ; i < 1000 ; ++i );
		DMA1_Stream5->CR &=~(1<<0);
		DMA1_Stream5->PAR = (uint32_t)&I2C1->DR;
		DMA1_Stream5->CR |= (0b001<<25)|(0b01<<16)|(0b00<<13)|(0b00<<11)|(1<<10)|(0b00<<6)|(1<<4);
		NVIC_SetPriority(DMA1_Stream5_IRQn,6);
		NVIC_EnableIRQ(DMA1_Stream5_IRQn);
}
extern "C" void I2C1_ER_IRQHandler()
{
		BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t sr1 = I2C1->SR1;

    if(sr1 & (1<<1)) 
    {
        (void)I2C1->SR1; 
        I2C1->CR1 |= IIC_STOP; 
    }
		xEventGroupClearBits(iic_event,IIC_BTF);
		xEventGroupClearBits(iic_event,SEND);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}
