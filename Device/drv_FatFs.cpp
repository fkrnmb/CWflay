#include "drv_FatFs.hpp"
#include "diskio.h"
#include "ff.h"
#include "ExtFlash.hpp"
#include "FreeRTOS.h"
#include "task.h"
#include "stdio.h"
#include "string.h"
#include "Time.hpp"
#define DEV_FLASH 0
#define DEV_MMC 1
#define DEV_USB 2

#define SPI_FLASH_SECTOR_SIZE 512
#define SPI_FLASH_SECTOR_COUNT 12 * 1024 * 2
#define SPI_FLASH_BLOCK_SIZE 8
#define SPI_FLASH_FATFS_BASE 0

// 获取SPI flash状态
DSTATUS disk_status(
		BYTE pdrv)
{
	DSTATUS stat;
	int result;

	switch (pdrv)
	{
	case DEV_FLASH:
	{
		if (Read_id() != 0XEF16)
			stat = RES_ERROR;
		else
			stat = RES_OK;
	}
		return stat;
	}
	return STA_NOINIT;
}

// 初始化spi
DSTATUS disk_initialize(
		BYTE pdrv)
{
	DSTATUS stat;
	uint8_t res = 0;
	;

	switch (pdrv)
	{
	case DEV_FLASH:
	{
		NORFLASH_Init();
		res = RES_OK;
		break;
	}
	}
	if (res)
	{
		return STA_NOINIT;
	}
	else
	{
		return 0;
	}
}

// 读取扇区
DRESULT disk_read(
		BYTE pdrv,		
		BYTE *buff,		
		LBA_t sector, 
		UINT count		
)
{
	DRESULT res;
	int result;

	switch (pdrv)
	{
		case DEV_FLASH:
		{
			for (; count > 0; count--)
			{
				ExtFLASHRead(buff, SPI_FLASH_FATFS_BASE + sector * SPI_FLASH_SECTOR_SIZE, SPI_FLASH_SECTOR_SIZE);
				sector++;
				buff += SPI_FLASH_SECTOR_SIZE;
			}
			res = RES_OK;
			break;
		}
		default:
     res = RES_ERROR;
	}

	  if (res == 0x00)
    {
        return RES_OK;
    }
    else
    {
        return RES_ERROR; 
    }
}

#if FF_FS_READONLY == 0
// 写操作
DRESULT disk_write(
		BYTE pdrv,
		const BYTE *buff,
		LBA_t sector,
		UINT count)
{
	DRESULT res;
	int result;

	switch (pdrv)
	{
		case DEV_FLASH:
		{
			for (; count > 0; count--)
			{
				ExtFlASHWiter((uint8_t *)buff, SPI_FLASH_FATFS_BASE + sector * SPI_FLASH_SECTOR_SIZE, SPI_FLASH_SECTOR_SIZE);
				sector++;
				buff += SPI_FLASH_SECTOR_SIZE;
			}
			res = RES_OK;
			break;
		}
		 default:
			res = RES_ERROR;
		 break;
	}

  if (res == 0x00)
  {
    return RES_OK;
  }
  else
  {
    return RES_ERROR; 
  }
}

#endif

DRESULT disk_ioctl(
		BYTE pdrv,
		BYTE cmd,
		void *buff)
{
	DRESULT res;
	int result;

	switch (cmd)
	{
	case CTRL_SYNC:
	{
		res = RES_OK;
		break;
	}
	case GET_SECTOR_SIZE:
	{
		*(WORD *)buff = SPI_FLASH_SECTOR_SIZE;
		res = RES_OK;
		break;
	}
	case GET_BLOCK_SIZE:
	{
		*(WORD *)buff = SPI_FLASH_BLOCK_SIZE;
		res = RES_OK;
		break;
	}
	case GET_SECTOR_COUNT:
	{
		*(DWORD *)buff = SPI_FLASH_SECTOR_COUNT;
		res = RES_OK;
		break;
	}
	default:
	{
		res = RES_PARERR;
		break;
	}
	}

	 return res;
}
DWORD get_fattime()
{
	return ((2026 - 1980) << 25) | ((1) << 21) | ((1) << 16) | ((1) << 11) | ((1) << 5) | ((1) << 0);
}
static int is_txt_file(const char *name)
{
    const char *dot = strrchr(name, '.');
    if (dot == NULL) return 0;

    return (strcmp(dot, ".txt") == 0);
}
FRESULT delete_dir_recursive(const char *path)
{
    FRESULT res;
    DIR dir;
    FILINFO fno;
    char fullpath[256];

    res = f_opendir(&dir, path);
    if (res != FR_OK)
        return res;

    while (1)
    {
        res = f_readdir(&dir, &fno);
        if (res != FR_OK || fno.fname[0] == 0)
            break;

        
        if (strcmp(fno.fname, ".") == 0 || strcmp(fno.fname, "..") == 0)
            continue;

        snprintf(fullpath, sizeof(fullpath), "%s/%s", path, fno.fname);

        if (fno.fattrib & AM_DIR)
        {
            // 递归删除子目录
            res = delete_dir_recursive(fullpath);
            if (res != FR_OK)
            {
                f_closedir(&dir);
                return res;
            }
        }
        else
        {
            // 删除文件
           if (is_txt_file(fno.fname))
						{
								res = f_unlink(fullpath);
								if (res != FR_OK)
								{
										f_closedir(&dir);
										return res;
								}
						}
        }
    }

    f_closedir(&dir);

    // 最后删除空目录本身
    return f_unlink(path);
}
FATFS flashfat;
FIL fil;
BYTE work[FF_MAX_SS];

//写入测试

uint8_t Rxbuf[3];
UINT numofwrite = 0;



static void  FatFS_Task_server(void* pvParameters)
{
	while(1)
	{

	}
}
void drv_FatFS_init()
{

	FRESULT res;
	FILINFO finfo;
	//挂载一个文件系统到一个设备上
	res = f_mount(&flashfat,"",1);
	if(res == FR_NO_FILESYSTEM)
	{
		res = f_mkfs("", 0, work,sizeof(work));
	}
	res = f_mount(&flashfat, "", 1);
	if(res == FR_OK)//创建一个文件夹
	{   
		res = f_mkdir("Flay");
	}
	xTaskCreate( FatFS_Task_server , "fa_Task_server" ,1024,NULL,1,NULL);
}