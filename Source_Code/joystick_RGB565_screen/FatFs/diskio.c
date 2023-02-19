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
#define SDcard		0		/* SD��, ���Ϊ0 */
//#define EX_Flash		1		/* ����Flash, ���Ϊ1 */


/*-----------------------------------------------------------------------*/
/* ��ô���״̬                                                          */
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
/* ��ʼ������                                                            */
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
/* ������                                                                */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* �̷����0~9	*/
	BYTE *buff,		/* ����buffer 	*/
	DWORD sector,	/* Sector�׵�ַ */
	UINT count		/* Sector�ĸ��� */
)
{
	DRESULT res = RES_PARERR;
	SD_Error SD_stat = SD_OK;

	if(count==0)	return RES_PARERR;	//count���ܵ���0�����򷵻ز�������
	else ;
	
	switch (pdrv) {
		case SDcard :
			if( (DWORD)buff & 3)	//���buff��ַû��4�ֽڶ���
			{
				DWORD alignbuff[SDCardInfo.CardBlockSize / 4];	//DWORD is 32bit = 4Byte
				//�����Լ�
				while(count--)
				{
					res = disk_read(SDcard, (BYTE*)alignbuff, sector++, 1);	//�����ݶ���alignbuff��
					if(res != RES_OK)
						break;
					memcpy(buff, alignbuff, SDCardInfo.CardBlockSize);	//��alignbuff���Ѷ��������ת�Ƶ�buff��
					buff += SDCardInfo.CardBlockSize;
				}
				return res;
			}
			else	//����4�ֽڶ���
			{
				SD_stat = SD_ReadMultiBlocks(buff, (u32)sector*SDCardInfo.CardBlockSize, SDCardInfo.CardBlockSize, count);
				if(SD_stat==SD_OK)
				{
					res = RES_OK;
					/* Check if the Transfer is finished */
					SD_stat = SD_WaitReadOperation();
					/* Wait until end of DMA transfer */
					while(SD_GetStatus() != SD_TRANSFER_OK);	//SD���������
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
/* д����                                                                */
/*-----------------------------------------------------------------------*/

#if _USE_WRITE
DRESULT disk_write (
	BYTE pdrv,			/* �̷����0~9  */
	const BYTE *buff,	/* ����buffer 	*/
	DWORD sector,		/* Sector�׵�ַ */
	UINT count			/* Sector�ĸ��� */
)
{
	DRESULT res = RES_PARERR;
	SD_Error SD_stat = SD_OK;

	if(count==0)	return RES_PARERR;	//count���ܵ���0�����򷵻ز�������
	else ;
	
	switch (pdrv) {
		case SDcard :
			if( (DWORD)buff & 3)	//���buff��ַû��4�ֽڶ���
			{
				DWORD alignbuff[SDCardInfo.CardBlockSize / 4];	//DWORD is 32bit = 4Byte
				//�����Լ�
				while(count--)
				{
					memcpy(alignbuff, buff, SDCardInfo.CardBlockSize);		//��buff��������alignbuff�ж���
					res = disk_write(SDcard, (BYTE*)alignbuff, sector++, 1);//��alignbuff�е����ݷ��ͳ�ȥ
					if(res != RES_OK)
						break;
					buff += SDCardInfo.CardBlockSize;
				}
				return res;
			}
			else	//����4�ֽڶ���
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
					while(SD_GetStatus() != SD_TRANSFER_OK);	//SD���������
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
/* ���һЩ������Ϣ(Miscellaneous Functions)                             */
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



//ʱ���
//User defined function to give a current time to fatfs module      */
//31-25: Year(0-127 org.1980), 24-21: Month(1-12), 20-16: Day(1-31) */                                                                                                                                                                                                                                          
//15-11: Hour(0-23), 10-5: Minute(0-59), 4-0: Second(0-29 *2)      	*/
DWORD get_fattime(void)
{
	/* �̶����� 2019-1-1  00:00:01 */
	return	( (DWORD)(2019-1980)<<25 )	/* Year  2019 */
		  |	( (DWORD)1<<21 )			/* Month  01 */
		  |	( (DWORD)1<<16 )			/* Day    01 */
		  |	( (DWORD)0<<11 )			/* Hour   00 */
		  |	( (DWORD)0<<5  )			/* Minute 00 */
		  |	( (DWORD)1<<0  );			/* Second 00 */
}



