#include "HeaDingSensor.hpp"
#include "stdint.h"
#include "drv_IIC.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "system.hpp"
#include <stdio.h>
#include "Uart2.hpp"
#include "Uart1.hpp"
#include "semphr.h"
#include "Time.hpp"
#include "stdio.h"

//磁力计互斥锁
static SemaphoreHandle_t mAgMutex = xSemaphoreCreateRecursiveMutex();

struct ak8975tmp
{
	int16_t Hx;
	int16_t Hy;
	int16_t Hz;
};

static SonerAkMagData mAg;

static void mAgRawDataUpdata(int16_t hx,int16_t hy,int16_t hz)
{
	mAg.Hx = hz;
	mAg.Hy = hy,
	mAg.Hz = hz;
}

//获取磁力计数据
bool GetmAgData(SonerAkMagData*mAgData,float TIMEOUT)
{
	uint32_t waitTicks;
	if(TIMEOUT >= 0)
		waitTicks = TIMEOUT*configTICK_RATE_HZ;
	else
		waitTicks = portMAX_DELAY;
	if(xSemaphoreTakeRecursive(mAgMutex,waitTicks)== pdTRUE )
	{
		*mAgData = mAg;
		xSemaphoreGiveRecursive(mAgMutex);
		return true;
	}
	else
		return false;
}

static void  HeadingSensor_Task_server(void* pvParameters)
{
  //启动磁力计
  uint8_t Mode = 0x01;
	IIC_SendData(AK8975_ADDRESS, AK8975_CNTL,&Mode,1,0.5);
  os_delay(1);

  uint8_t buf[6];
  ak8975tmp ak;
  while(1)
  {
    IIC_ReceiveAdder7(AK8975_ADDRESS,AK8975_HXL,buf,6);
    ak.Hx = (buf[1]<<8)|buf[0];
		ak.Hy = (buf[3]<<8)|buf[2];
		ak.Hz = (buf[5]<<8)|buf[4];
    struct aKmAgddata
		{
			int16_t ax;
			int16_t ay;
			int16_t az;
		}__attribute__((__packed__));
		aKmAgddata* mAgpack = (aKmAgddata*)&ak;
    mAgRawDataUpdata(mAgpack->ax,mAgpack->ay,mAgpack->az);
		IIC_SendData(AK8975_ADDRESS, AK8975_CNTL,&Mode,1);
		os_delay(1);
  }
}

void initHeadingSensor()
{
 xTaskCreate( HeadingSensor_Task_server , "MAG_Task_server" ,1024,NULL,3,NULL);
}


