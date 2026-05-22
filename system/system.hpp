#pragma once

#include "stm32f4xx.h"

#define USART1_CLK 84000000UL
#define USART2_CLK 42000000UL




inline void set_register(volatile unsigned int &reg, const unsigned char value, const unsigned char offset, const unsigned char value_length)
{
    unsigned char offset_end_bit = offset + value_length;
    for (unsigned char i = offset; i < offset_end_bit; ++i)
        reg &= ~(1 << i);

    reg |= (value << offset);
}


void systeamInit(void);




