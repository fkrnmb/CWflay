#include "IMU.hpp"
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
#include "drv_USB.hpp"
#include "Time.hpp"

//imp互斥锁
static SemaphoreHandle_t iMuMutex = xSemaphoreCreateRecursiveMutex();


static SoneriMuData imu ;



static void iMuRawDataUpdata(int16_t accx,int16_t accy,int16_t accz,int16_t temp
	,int16_t gyrox,int16_t gyroy,int16_t gyroz,float s1,float s2)
{
	imu.accx = accx;
	imu.accy = accy;
	imu.accz = accz;
	imu.gyrox = gyrox;
	imu.gyroy = gyroy;
	imu.gyroz = gyroz;
	imu.temp = temp;
	imu.Accsens = s1;
	imu.Gyrosens = s2;
}


bool GetiMuData(SoneriMuData*iMu,float TIMEOUT)
{
	uint32_t waitTicks;
	if(TIMEOUT >= 0)
		waitTicks = TIMEOUT*configTICK_RATE_HZ;
	else
		waitTicks = portMAX_DELAY;
	if(xSemaphoreTakeRecursive(iMuMutex,waitTicks)== pdTRUE )
	{
		*iMu = imu;
		xSemaphoreGiveRecursive(iMuMutex);
		return true;
	}
	else
		return false;
		
}



static void  IMU_Task_server(void* pvParameters)
{
	
//	/*启动mpu6050*/
	uint8_t data1 = 0x00;
	uint8_t data2 = 0x09;
	uint8_t data3 = 0x18;
	IIC_SendData(MPU6050_ADDR,0x6b,&data1,1);
	IIC_SendData(MPU6050_ADDR,MPU6050_SMPLRT_DIV,&data2,1);
	IIC_SendData(MPU6050_ADDR,MPU6050_ACCEL_CONFIG,&data3,1); //+-16g
	IIC_SendData(MPU6050_ADDR,MPU6050_GYRO_CONFIG,&data3,1); //+-2000
	os_delay(1);
	/*启动mpu6050*/

	struct mpudatatmp
	{
		int16_t ax;
		int16_t ay;
		int16_t az;
		int16_t temp;
		int16_t gx;
		int16_t gy;
		int16_t gz;
		float s1;
		float s2;
	};
	mpudatatmp mpu6050;
	uint8_t buf[14];
	
	while(1)
	{
		struct mpu6050Data
		{
			int16_t ax;
			int16_t ay;
			int16_t az;
			int16_t temp;
			int16_t gx;
			int16_t gy;
			int16_t gz;
			float s1;
			float s2;
		}__attribute__((__packed__));

		IIC_ReceiveAdder7(MPU6050_ADDR,MPU6050_ACCEL_XOUT_H,buf,14);

		mpu6050.ax = (buf[0]<<8)| buf[1];
		mpu6050.ay = (buf[2]<<8)| buf[3];
		mpu6050.az = (buf[4]<<8) |buf[5];
		mpu6050.temp = (buf[6]<<8) |buf[7];
		mpu6050.gx = (buf[8]<<8) | buf[9];
		mpu6050.gy = (buf[10]<<8) | buf[11];
		mpu6050.gz = (buf[12]<<8)| buf[13];
		//更新
		mpu6050.s1 = mpu6050.ax/2048;
		mpu6050.s2 = mpu6050.gx/16.38;
		mpu6050Data*data = (mpu6050Data*)&mpu6050;
		iMuRawDataUpdata(data->ax,data->ay,data->az,
		data->gx,data->gy,data->gz,data->temp,data->s1,data->s2);
		os_delay(1);
	}
}




void IMU_Init()
{
	//创建任务
	xTaskCreate( IMU_Task_server , "IMU_Task_server" ,1400,NULL,3,NULL);
}