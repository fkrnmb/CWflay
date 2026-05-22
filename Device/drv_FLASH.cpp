#include "drv_FLASH.hpp"
#include "stm32f4xx.h"

#define MAINSTPRGAE 0X1FFF0000


//获取指定地址所在扇区
static uint8_t GetFlashSector(uint32_t addr)
{

	if(addr<ADDR_FLASH_SECTOR_1)		
		return 0;
	else if(addr<ADDR_FLASH_SECTOR_2)	
		return 1;
	else if(addr<ADDR_FLASH_SECTOR_3)
		return 2;
	else if(addr<ADDR_FLASH_SECTOR_4)	
		return 3;
	else if(addr<ADDR_FLASH_SECTOR_5)	
		return 4;
	else if(addr<ADDR_FLASH_SECTOR_6)	
		return 5;
	else if(addr<ADDR_FLASH_SECTOR_7)
		return 6;
	else if(addr<ADDR_FLASH_SECTOR_8)	
		return 7;
	else if(addr<ADDR_FLASH_SECTOR_9)	
		return 8;
	else if(addr<ADDR_FLASH_SECTOR_10)	
		return 9;
	else if(addr<ADDR_FLASH_SECTOR_11)	
		return 10; 
	else return 11;

}

/*
	指定地址读取一个字节
addr:读取的地址 必须是4的倍数
*/

uint32_t FlashReadWord(uint32_t addr)
{
	 return *(volatile uint32_t *)addr;
}
/*
指定位置写入指定长度的数据:自动擦除
*/
bool FlashWrite(uint32_t addr,uint32_t*buf,uint16_t length)
{

	if (addr < FLASH_FLASH_BASE || addr % 4 ||addr> (FLASH_FLASH_BASE + FLASH_SIZE)) 
		return false;
  FLASH_EraseInitTypeDef flasheraseinit;
  HAL_StatusTypeDef FlashStatus=HAL_OK;
	uint32_t addrx = 0;
  uint32_t endaddr = 0;
  uint32_t sectorerror=0;
	//解锁
	HAL_FLASH_Unlock();
	FLASH->ACR &= ~(1 << 10); //擦除期间禁止缓存
	
	addrx = addr; 
	endaddr = addr + length * 4; 
	//主储存区执行擦除操作
	if(addrx  <MAINSTPRGAE)
	{ 
		while(addrx < endaddr)
		{
			if (FlashReadWord(addrx) != 0XFFFFFFFF)  
			{
				flasheraseinit.TypeErase=FLASH_TYPEERASE_SECTORS;
				flasheraseinit.Sector=GetFlashSector(addrx);  
				flasheraseinit.NbSectors = 1;				//一次擦一个
				flasheraseinit.VoltageRange=FLASH_VOLTAGE_RANGE_3;  //范围
				if(HAL_FLASHEx_Erase(&flasheraseinit, &sectorerror) != HAL_OK) 
					break;
			}
			else
				addrx+=4;
			//等待上次操完成
			FLASH_WaitForLastOperation(FLASH_WAITETIME);
		}
	}
	//等待上次操完成
	FlashStatus=FLASH_WaitForLastOperation(FLASH_WAITETIME); 
	//写数据
	if (FlashStatus==HAL_OK)
	{
		while(addr <endaddr)
		{
			if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, addr, *buf) != HAL_OK) 
				break;
			addr += 4;
			buf++;
		}
	}
		//擦除结束开启数据
	FLASH->ACR |= (1<<10);
	HAL_FLASH_Lock(); 	
	return true;
}

/*
指定地址读出指定长度数据
addr 地址
buf 数据
length 要读的字32位数 4字节
*/

void FlashRead(uint32_t addr,uint32_t *buf,uint32_t length)
{
	for(uint32_t i = 0;i<length;++i)
	{
		buf[i] = FlashReadWord(addr);
		addr += 4;
	}
}
