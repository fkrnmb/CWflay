#pragma once
#include <stdint.h>


bool IIC_ReceiveAdder7(uint8_t addr,uint8_t reg,uint8_t*data,uint16_t len,float TIME_OUT = 0 );
bool IIC_SendData(uint8_t addr,uint8_t reg,uint8_t*data,uint16_t len ,float TIME_OUT = 0 );
void IIC_Init();


