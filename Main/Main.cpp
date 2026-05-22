#include "system.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "drv_USB.hpp"
#include "stdint.h"
#include "drv_SPI.hpp"
#include "ExtFlash.hpp"
#include "drv_FLASH.hpp"
#include "Communication.hpp"

#include "Uart1.hpp"
#include "Uart2.hpp"
#include "usb_msc.h"
#include "drv_LED.hpp"
#include "stdio.h"
#include "Time.hpp"
#include "drv_IIC.hpp"
#include "IMU.hpp"
#include "HeaDingSensor.hpp"
#include "Debug.hpp"
#include "drv_FatFs.hpp"


void Task_StartInit(void* pvParameters)
{

		IMU_Init();
		initHeadingSensor();
		drv_Init_Debug();
    vTaskDelete(NULL); 
}
int main(void)
{
	//系统初始化
	systeamInit();
	
	InitTime();
	Init_Usart1();
	Init_Usart2();
	IIC_Init();
	drv_spiInit();
	drv_LED_Init();

	drv_Init_Usb();
	drv_FatFS_init();
	xTaskCreate( Task_StartInit , "Init" ,1024,NULL,3,NULL);
	vTaskStartScheduler();
	while(1);
}

//发生故障
extern "C" void HardFault_Handler()
{
	
}


      