#include "ExtFlash.hpp"
#include "drv_SPI.hpp"
#include "stm32f4xx_hal.h"
#include "stdio.h"

uint16_t g_norflash_type = W25Q64 ;

static uint8_t ExtFlASHRead_Rr(uint8_t regno)
{
  uint8_t byte = 0, command = 0;

  switch (regno)
  {
      case 1:
      {
        command = FLASH_ReadStatusReg1; 
        break;
      } 
      case 2:
      {
        command = FLASH_ReadStatusReg2;  
        break;
      }    
      case 3:
      {
        command = FLASH_ReadStatusReg3; 
        break;
      }     
      default:
      {
        command = FLASH_ReadStatusReg1;
        break;
      }   
        
  }

  NORFLASH_CS(0);
  spiReadAndWriteByte(command);     
  byte = spiReadAndWriteByte(0Xff);  
  NORFLASH_CS(1);
  
  return byte;
}

static void ExtFlASHWrite_Rr(uint8_t regno, uint8_t sr)
{
  uint8_t command = 0;

  switch (regno)
  {
      case 1:
      {
        command = FLASH_WriteStatusReg1;  
        break;
      }    
      case 2:
      {
        command = FLASH_WriteStatusReg2;  
        break;
      }   
      case 3:
      {
         command = FLASH_WriteStatusReg3;  
         break;
      }  
      default:
       {
        command = FLASH_WriteStatusReg1;  
         break;
       }   
  }
  NORFLASH_CS(0);
  spiReadAndWriteByte(command);     
  spiReadAndWriteByte(sr);  
  NORFLASH_CS(1);
}
static void norflash_wait_busy(void)
{
  while ((ExtFlASHRead_Rr(1) & 0x01) == 0x01);
}
//写使能
void ExtFLASHWriteEnable()
{
 NORFLASH_CS(0);
 spiReadAndWriteByte(FLASH_WriteEnable);  
 NORFLASH_CS(1);
}
//发送地址
static void norflash_send_address(uint32_t address)
{
  if (g_norflash_type == W25Q256)                    
  {
      spiReadAndWriteByte((uint8_t)((address)>>24)); 
  } 
  spiReadAndWriteByte((uint8_t)((address)>>16));     
  spiReadAndWriteByte((uint8_t)((address)>>8));      
  spiReadAndWriteByte((uint8_t)address);             
}
//擦除一个扇区
void norflash_erase_sector(uint32_t saddr)
{

  saddr *= 4096;
  ExtFLASHWriteEnable();       
  norflash_wait_busy();           

  NORFLASH_CS(0);
  spiReadAndWriteByte(FLASH_SectorErase);  
  norflash_send_address(saddr);  
  NORFLASH_CS(1);
  norflash_wait_busy();         
}
uint16_t Read_id(void)
{
  uint16_t deviceid;

  NORFLASH_CS(0);
  spiReadAndWriteByte(FLASH_ManufactDeviceID);   
  spiReadAndWriteByte(0);                    
  spiReadAndWriteByte(0);
  spiReadAndWriteByte(0);
  deviceid = spiReadAndWriteByte(0xFF) << 8;    
  deviceid |= spiReadAndWriteByte(0xFF);     
  NORFLASH_CS(1);

  return deviceid;
}





//初始化 NORFLASH
void NORFLASH_Init()
{
	uint8_t temp;
	NORFLASH_CS_GPIO_CLK_ENABLE(); //CS 引脚
	GPIO_InitTypeDef gpio_init_struct;
  gpio_init_struct.Pin = NORFLASH_CS_GPIO_PIN;
  gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init_struct.Pull = GPIO_PULLUP;
  gpio_init_struct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(NORFLASH_CS_GPIO_PORT, &gpio_init_struct);  //CS 引脚初始化
	NORFLASH_CS(1); //CS 取消片选
	spiSetspeed(SPI_SPEED_4);//高速
  g_norflash_type = Read_id();
  printf("ID = %X\r\n", g_norflash_type);
  if (g_norflash_type == W25Q64)  
  {
    if ((temp & 0X01) == 0)   //不是4字节地址模式
    {
      ExtFLASHWriteEnable();
      temp |= 1 << 1;  
      ExtFlASHWrite_Rr(3, temp);
      NORFLASH_CS(0);
      spiReadAndWriteByte(FLASH_Enable4ByteAddr); 
      NORFLASH_CS(1);
    }
  }
}


//读取spiFALSH
void ExtFLASHRead(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    NORFLASH_CS(0);
    spiReadAndWriteByte(FLASH_ReadData);      
    norflash_send_address(addr);                
    
    for (i = 0; i < datalen; i++)
    {
        pbuf[i] = spiReadAndWriteByte(0XFF);   
    }
    
    NORFLASH_CS(1);
}

//在一页内写入数据 少于256字节
static void norflash_write_page(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t i;

    ExtFLASHWriteEnable();                    

    NORFLASH_CS(0);
    spiReadAndWriteByte(FLASH_PageProgram);   
    norflash_send_address(addr);              

    for (i = 0; i < datalen; i++)
    {
        spiReadAndWriteByte(pbuf[i]);     
    }
    
    NORFLASH_CS(1);
    norflash_wait_busy();       
}

//无检查写FLASH
static void norflash_write_nocheck(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint16_t pageremain;
    pageremain = 256 - addr % 256; 

    if (datalen <= pageremain)     
    {
        pageremain = datalen;
    }

    while (1)
    {

        norflash_write_page(pbuf, addr, pageremain);

        if (datalen == pageremain)      
        {
            break;
        }
        else                           
        {
            pbuf += pageremain;        
            addr += pageremain;        
            datalen -= pageremain;     

            if (datalen > 256)         
            {
                pageremain = 256;      
            }
            else                        
            {
                pageremain = datalen;  
            }
        }
    }
}

//写flash 
uint8_t g_norflash_buf[4096]; 

void ExtFlASHWiter(uint8_t *pbuf, uint32_t addr, uint16_t datalen)
{
    uint32_t secpos;
    uint32_t  secoff;
    uint32_t  secremain;
    uint32_t  i;
    uint8_t *norflash_buf;

    norflash_buf = g_norflash_buf;
    secpos = addr / 4096;     
    secoff = addr % 4096;    
    secremain = 4096 - secoff;  

 
    if (datalen <= secremain)
    {
        secremain = datalen;    
    }

    while (1)
    {
        ExtFLASHRead(norflash_buf, secpos * 4096, 4096); 

        for (i = 0; i < secremain; i++)    
        {
            if (norflash_buf[secoff + i] != 0XFF)
            {
                break;               
            }
        }

        if (i < secremain)            
        {
            norflash_erase_sector(secpos);  

            for (i = 0; i < secremain; i++) 
            {
                norflash_buf[i + secoff] = pbuf[i];
            }

            norflash_write_nocheck(norflash_buf, secpos * 4096, 4096); 
        }
        else    
        {
            norflash_write_nocheck(pbuf, addr, secremain);            
        }

        if (datalen == secremain)
        {
            break;  
        }
        else        
        {
            secpos++;              
            secoff = 0;            

            pbuf += secremain;      
            addr += secremain;      
            datalen -= secremain;   

            if (datalen > 4096)
            {
                secremain = 4096;  
            }
            else
            {
                secremain = datalen;
            }
        }
    }
}

//擦除所有
void norflash_erase_chip(void)
{
    ExtFLASHWriteEnable();   
    norflash_wait_busy();      
    NORFLASH_CS(0);
    spiReadAndWriteByte(FLASH_ChipErase);  
    NORFLASH_CS(1);
    norflash_wait_busy();   
}
