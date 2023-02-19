/*-----------------------------------------------------------------------*/
/* Low level disk I/O module skeleton for FatFs     (C)ChaN, 2014        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "diskio.h"		/* FatFs lower layer API */
#include "sdio_sdcard.h"
#include <string.h>		/* To use memcpy() function */

extern SD_CardInfo SDCardInfo;

/* Definitions of physical drive number for each drive */
#define SDcard		0		/* SD卡, 卷标为0 */
//#define EX_Flash		1		/* 外扩Flash, 卷标为1 */


/*-----------------------------------------------------------------------*/
/* 获得磁盘状态                                                          */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
		case SDcard :
			stat &= ~STA_NOINIT;
			break;
/*
		case EX_Flash :
			break;
*/
	}
	return stat;
}



/*-----------------------------------------------------------------------*/
/* 初始化磁盘                                                            */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;

	switch (pdrv) {
		case SDcard :
			if(SD_Init()==SD_OK)
				stat &= ~STA_NOINIT;
			else
				stat = STA_NOINIT;
			break;
/*
		case EX_Flash :
			break;
*/
	}
	return stat;
}



/*-----------------------------------------------------------------------*/
/* 读扇区                                                                */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* 盘符编号0~9	*/
	BYTE *buff,		/* 接收buffer 	*/
	DWORD sector,	/* Sector首地址 */
	UINT count		/* Sector的个数 */
)
{
	DRESULT res = RES_PARERR;
	SD_Error SD_stat = SD_OK;

	if(count==0)	return RES_PARERR;	//count不能等于0，否则返回参数错误
	else ;
	
	switch (pdrv) {
		case SDcard :
			if( (DWORD)buff & 3)	//如果buff地址没有4字节对齐
			{
				DWORD alignbuff[SDCardInfo.CardBlockSize / 4];	//DWORD is 32bit = 4Byte
				//调用自己
				while(count--)
				{
					res = disk_read(SDcard, (BYTE*)alignbuff, sector++, 1);	//把数据读到alignbuff中
					if(res != RES_OK)
						break;
					memcpy(buff, alignbuff, SDCardInfo.CardBlockSize);	//把alignbuff里已对齐的数据转移到buff中
					buff += SDCardInfo.CardBlockSize;
				}
				return res;
			}
			else	//若已4字节对齐
			{
				SD_stat = SD_ReadMultiBlocks(buff, (u32)sector*SDCardInfo.CardBlockSize, SDCardInfo.CardBlockSize, count);
				if(SD_stat==SD_OK)
				{
					res = RES_OK;
					/* Check if the Transfer is finished */
					SD_stat = SD_WaitReadOperation();
					/* Wait until end of DMA transfer */
					while(SD_GetStatus() != SD_TRANSFER_OK);	//SD卡传输结束
				}
				else
					res = RES_ERROR;
			}
			break;
/*
		case EX_Flash :
			break;
*/
	}
	return res;
}



/*-----------------------------------------------------------------------*/
/* 写扇区                                                                */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* 盘符编号0~9  */
	const BYTE *buff,	/* 发送buffer 	*/
	DWORD sector,		/* Sector首地址 */
	UINT count			/* Sector的个数 */
)
{
	DRESULT res = RES_PARERR;
	SD_Error SD_stat = SD_OK;

	if(count==0)	return RES_PARERR;	//count不能等于0，否则返回参数错误
	else ;
	
	switch (pdrv) {
		case SDcard :
			if( (DWORD)buff & 3)	//如果buff地址没有4字节对齐
			{
				DWORD alignbuff[SDCardInfo.CardBlockSize / 4];	//DWORD is 32bit = 4Byte
				//调用自己
				while(count--)
				{
					memcpy(alignbuff, buff, SDCardInfo.CardBlockSize);		//把buff数据置入alignbuff中对齐
					res = disk_write(SDcard, (BYTE*)alignbuff, sector++, 1);//把alignbuff中的数据发送出去
					if(res != RES_OK)
						break;
					buff += SDCardInfo.CardBlockSize;
				}
				return res;
			}
			else	//若已4字节对齐
			{
				if(count == 1)
					SD_stat = SD_WriteBlock((u8*)buff, (u32)sector*SDCardInfo.CardBlockSize, SDCardInfo.CardBlockSize);
				else
					SD_stat = SD_WriteMultiBlocks((u8*)buff, (u32)sector*SDCardInfo.CardBlockSize, SDCardInfo.CardBlockSize, count);
				if(SD_stat==SD_OK)
				{
					res = RES_OK;
					/* Check if the Transfer is finished */
					SD_stat = SD_WaitWriteOperation();
					/* Wait until end of DMA transfer */
					while(SD_GetStatus() != SD_TRANSFER_OK);	//SD卡传输结束
				}
				else
					res = RES_ERROR;
			}
			break;
/*
		case EX_Flash :
			break;
*/
	}
	return res;
}
#endif


/*-----------------------------------------------------------------------*/
/* 获得一些杂项信息(Miscellaneous Functions)                             */
/*-----------------------------------------------------------------------*/

#if _USE_IOCTL
DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Arg_Buffer to send/receive control data */
)
{	
	switch (pdrv) {
		case SDcard :
			switch(cmd)
			{
				case CTRL_SYNC :
					break;
				case GET_SECTOR_COUNT :
					*(DWORD*)buff = SDCardInfo.CardCapacity / SDCardInfo.CardBlockSize;
					break;
				case GET_SECTOR_SIZE :
					*(WORD *)buff = SDCardInfo.CardBlockSize;
					break;
				/* erase block size of the flash memory media (in unit of sector) */
				case GET_BLOCK_SIZE :
					*(DWORD*)buff = 1;
					break;
				case CTRL_TRIM :
					break;
				default:	break;
			}	
			break;
/*
		case EX_Flash :
			break;
*/
	}
	return RES_OK;
}
#endif



//时间戳
//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2)      	*/
DWORD get_fattime(void)
{
	/* 固定返回 2019-1-1  00:00:01 */
	return	( (DWORD)(2019-1980)<<25 )	/* Year  2019 */
		  |	( (DWORD)1<<21 )			/* Month  01 */
		  |	( (DWORD)1<<16 )			/* Day    01 */
		  |	( (DWORD)0<<11 )			/* Hour   00 */
		  |	( (DWORD)0<<5  )			/* Minute 00 */
		  |	( (DWORD)1<<0  );			/* Second 00 */
}



