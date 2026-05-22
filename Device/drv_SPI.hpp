#pragma once
#include <stdint.h>

//spi速度
#define SPI_SPEED_2         0
#define SPI_SPEED_4         1
#define SPI_SPEED_8         2
#define SPI_SPEED_16        3
#define SPI_SPEED_32        4
#define SPI_SPEED_64        5
#define SPI_SPEED_128       6
#define SPI_SPEED_256       7




void drv_spiInit();
uint8_t spiReadAndWriteByte(uint8_t data);
void spiSetspeed(uint8_t speed);
