#pragma once
#include <cstdint>


#define AK8975_ADDRESS  0x0c	
#define AK8975_WIA     0x00
#define AK8975_HXL     0x03
#define AK8975_HXH     0x04
#define AK8975_HYL     0x05
#define AK8975_HYH     0x06
#define AK8975_HZL     0x07
#define AK8975_HZH     0x08
#define AK8975_CNTL    0x0A


struct SonerAkMagData
{
	int16_t Hx;
	int16_t Hy;
	int16_t Hz;
};
bool GetmAgData(SonerAkMagData*mAgData,float TIMEOUT = 0);
void initHeadingSensor();
