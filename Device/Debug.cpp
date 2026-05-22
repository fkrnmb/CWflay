#include "Debug.hpp"
#include "IMU.hpp"
#include "HeaDingSensor.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "Time.hpp"
#include "drv_USB.hpp"
#include "Communication.hpp"
#include "ExtFlash.hpp"
#include "string.h"
#include "ff.h"


static void  Debug_Task_server(void* pvParameters)
{
	

	
	while(1)
	{

		os_delay(100);
	}
}


void drv_Init_Debug()
{
		xTaskCreate( Debug_Task_server , "de_Task_server" ,800,NULL,1,NULL);
}


