#include "extfunc.h"
//#include "ff.h"
#include "sdio_sdcard.h"
#include "oled.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*��˵������
/  ��c�ļ������˻���FatFsģ�����չ����, ֻ�������ĺ������������oled.h		  *
/  ����ĵĺ���ֻ��������ef_getcapacity(), ef_scanfiles(), ef_checkfile()	  *
/  ����ĸ��Ӻ����������˺��ĺ���, ������Ҫ���ڼ� main.c , ��ֲʱ����ɾ��   */
/**
  ******************************************************************************
  *                             �������
  ******************************************************************************
  */
extern SD_CardInfo SDCardInfo;

FATFS 	fs;					//FatFs�ļ�ϵͳ�ṹ��ָ��
FIL		fp;					//�ļ��ṹ��ָ��
UINT	bw,br;				//��ǰ�ܹ�д��/��ȡ���ֽ���(����f_read(),f_write())
FILINFO	fno;				//�ļ���Ϣ
FRESULT	f_res;				//File function return code (FRESULT)
char	filename[15][127];	//���Ըĳ�malloc()
char 	cur_path[10]="0:";	//���浱ǰ���ڵ�·�� either "0:" or "0:/" is OK
u32 	tot_size=0, fre_size=0, sd_size=0;
//u8		fnum=0;				//·���µ��ļ�(��)����

//��Ϊ���ֱ������0x80=128>ascii, ���Ա���Ϊstr[]Ӧ�ö���� u8* ��ʽ
extern char	str[256];		//�洢��SD����txt�ļ��������ַ���
extern char bin_pic[680];	//�洢��SD����bin�ļ�������һ֡ͼ��

extern u16	ADC_Mem[10];
extern u8	left_key_value;
extern u8 	right_key_value;	//����ȡ��
extern u8	user1_key_value;
extern u8	user2_key_value;
extern u8	upper_item;
extern u8	lower_item;
extern u16	adc_thrust;
extern u16	adc_yaw;
extern u16	adc_pitch;			//ǰ������
extern u16	adc_roll;			//���Ҳ���
extern u16	adc_battery;
extern u8	Trig_oled_FPS;		// 25Hz


/**
  ******************************************************************************************************
  *                                         �Զ�����չ���� (����)
  ******************************************************************************************************
  */
/**
  * @brief  �õ���������
  * @param  drv_path: ����·��("0:"��"1:")
  *			* tot/fre_size: FatFs����(KB)
			* sd_size: SD����ȫ����������
  * @retval f_res: �ļ�ϵͳ����ֵFRESULT
  */
FRESULT ef_getcapacity(char* drv_path, u32* tot_size, u32* fre_size, u32* sd_size)
{
	FATFS *fs1;
	FRESULT	f_res;
	DWORD fre_clust, fre_sect, tot_sect;
	
	/* Get volume information and free clusters of drive 0 */
	f_res = f_getfree( (const TCHAR*)drv_path, &fre_clust, &fs1);
	if(f_res==FR_OK)
	{
		/* Get total sectors and free sectors */
		tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//��������
		fre_sect = fre_clust * fs1->csize;			//����������
		*tot_size = tot_sect>>1;	//��λ��KB
		*fre_size = fre_sect>>1;	//��λ��KB
#if	_MAX_SS != _MIN_SS
		*tot_size *= fs1->ssize * 4;	//��MAX_SS����Ϊ4096, �����4KB
		*fre_size *= fs1->ssize * 4;	//����KBǧ�ֽ���Ҳ����4��
#endif
		*sd_size = SDCardInfo.CardCapacity>>10;//��λ��KB
	}
	return f_res;
}


/**
  * @brief  ɨ�赱ǰĿ¼�µ������ļ���
  * @param  *path: ɨ��·��
  *			*num:  ��·���µ���Ŀ����(�����ļ��к��ļ�)
  * @retval f_res: �ļ�ϵͳ����ֵFRESULT
  */
FRESULT ef_scanfiles(char* path, u8* num)
{
	FRESULT	f_res;
	DIR dir;		//Ŀ¼�ṹ��ָ��
	char* fn;
	u8 i = 0;		//·���µ��ļ�����
	//u8 len = 0;		//·�����ĳ���, �������·����
	
#if _USE_LFN 
  /* ���ļ���֧�� */
  /* ��������һ��������Ҫ2���ֽ� */
  static char lfn[_MAX_LFN*2 + 1]; 	
  fno.lfname = lfn; 
  fno.lfsize = sizeof(lfn); 
#endif 
	
	f_res = f_opendir(&dir, (const TCHAR*)path);//��Ŀ¼
	if(f_res==FR_OK)
	{
		//len = strlen(path);
//		sprintf(cur_path, "%s%s", path, "/");//��·���Ľ�β׷��һ��slash
		//strcpy(cur_path, path);
		//strcat(cur_path, "/");//��·���Ľ�β׷��һ��slash
		while(1)
		{
			f_res = f_readdir(&dir, &fno);		//����ǰĿ¼, �ٴζ�ȡ���Զ�����һ���ļ�
			if(f_res!=FR_OK || fno.fname[0]==0)	//��ȡ����/����ĩβ(���ؿ�)ʱ����
			//{
				//if(i == 0)	strcpy(filename[0], "\0");	//���һ���ļ�Ҳ������, ����Ӧ������һ��\0
				break;
			//}
//			if(fno.lfname[0]=='.')	continue;	//�����ļ���׺��
#if _USE_LFN 
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
			if(fno.fattrib & AM_HID)	//It is a hidden Dir/File
				continue;				//����, ����������num
//			if(fno.fattrib & AM_DIR)	//It is a directory
//				strcat(filename[i],"+");//���ļ�������ͷ������"+"
//			else						//It is a file
//				strcat(filename[i]," ");//���ļ�������ͷ������" "
//			strcat(filename[i], fn);
			strncpy(filename[i], fn, 126);	//filename�ܳ�127, ��ֹ�ڴ���������126
			filename[i][126]='\0';			//��ĩβ��127�ֽڲ���\0��֤����һ��������
			i++;
		}
		*num = i;
		f_closedir(&dir);	//�رյ�ǰĿ¼
	}
	return f_res;
}



/**
  * @brief  ��ȡ�ļ�(��)����
  * @note	ע��˺����ܹ������ļ���ȫ������, ��������ֵ�����Ǳ�ʶ!
  * @param  *path: �ļ�(��)��·��(Ҫ�����ļ����ͺ�׺)
  * @retval file_attribute: �ļ�(��)���Ա�ʶ
  */
u8 ef_checkfile(char* path)
{
	FRESULT f_res = f_stat((const TCHAR*)path, &fno);
	if(f_res==0)
	{
/*		printf(">>�ļ���С: %ld(Byte)\n", fno.fsize);
		printf(">>�޸�ʱ��: %u/%02u/%02u, %02u:%02u\n",(fno.fdate >> 9) + 1980,
			fno.fdate >> 5 & 15, fno.fdate & 31,fno.ftime >> 11, fno.ftime >> 5 & 63);
		printf(">>����: %c%c%c%c%c\n\n",
           (fno.fattrib & AM_DIR) ? 'D' : '-',      //Director(�ļ���/Ŀ¼)
           (fno.fattrib & AM_RDO) ? 'R' : '-',      //Read-Only
           (fno.fattrib & AM_HID) ? 'H' : '-',      //Hidden
           (fno.fattrib & AM_SYS) ? 'S' : '-',      //System
           (fno.fattrib & AM_ARC) ? 'A' : '-');     //Archive(�����ļ�)
		*/
		return fno.fattrib;
	}
	else
		return 0;	//0��������
}




/**
  ******************************************************************************************************
  *                                        �Զ�����չ���� (����)
  ******************************************************************************************************
  */
/**
  * @brief  oled��ʾSD���ĸ�������
  * @note	�˺�����ֹѭ������(��Ҫ�ظ���дSD��)
  * @param  drv_path: �����
  * @retval None
  */
void ef_display_SDcapcity(char* drv_path)
{
//	if(SD_Init()==SD_OK)
//		oled_show_string(0, 52,"SD_OK.",12,1);
//	else
//		oled_show_num_hide0(0, 52, SD_Init(),2,12,1);
//	oled_show_num_hide0(42, 52, SDCardInfo.CardCapacity>>10,11,12,1);//SD������������
//	oled_show_num_hide0(84,  0, SDCardInfo.CardType,2,12,1);
	
	if(f_res==FR_OK)
	{
		oled_show_string(0, 0, (u8*)"FatFs ready. ",12,1);
		f_res = ef_getcapacity(drv_path, &tot_size, &fre_size, &sd_size);	//ef_getcapacity("0:", &tot_size, &fre_size, &sd_size);
		if(f_res==FR_OK)
		{
			oled_show_string(0, 18, (u8*)"SDcap:        KB",12,1);
			oled_show_num_hide0(36, 18, sd_size>>0, 7,12,0);//������ʾSD������������
			oled_show_string(0, 33, (u8*)"total:        KB",12,1);
			oled_show_num_hide0(36, 33, tot_size>>0,7,12,1);
			oled_show_string(0, 48, (u8*)"spare:        KB",12,1);
			oled_show_num_hide0(36, 48, fre_size>>0,7,12,1);
		}
		else
			oled_show_num_hide0(0, 33, f_res,2,12,0);	//������ʾ�����
	}
	else
		oled_show_num_hide0(0, 0, f_res,2,12,0);		//������ʾ�����
	//oled_refresh();
}



/**
  * @brief  oled��ʾ��ǰĿ¼�µ��ļ�
  * @note	�ڴ˺���֮ǰӦ��ʹ�� ef_scanfile() �����õ��ļ���fnum
  * @param  path: ��ǰ·��
			fnum: ��ǰ·���µ��ļ���
			frame:һ����Ļÿ���ܹ���ʾ���ļ�����
			j = j1 Ϊ��ǰѡ�е�������
			direct = delta ������ǰ���������»�������(�п���Խ���½�)
  * @retval None
  */
void ef_display_filelist(char* path, u8 fnum_t, u8 frame, u8 j, u8 direct, u8* head)
{
//*** ע��ÿһҳ���ļ��б�ֻ��ȡһ��, ��ʾ��������, �������ǲ�Ҫ����һ���ķ���SD�� ***
//	static u8 head = 1;
	if(f_res==FR_OK)
	{
		oled_buffer_clear(0, 0, 127, 63);
		oled_show_string(0, 0, (u8*)path,12,1);		//��ʾ��ǰ·��
		oled_draw_line(0, 13, 0, 63, 1);			//��ʾ���Ǧ����
		oled_show_num_hide0(110, 0, fnum_t,3,12,1);	//һ�������ļ�
		if(fnum_t <= frame)												(*head)= 1;				//����̫С, head����1
		else if((j==1)&&(*head==1)&&(direct==1))						(*head)= 1;				//Խ�Ͻ�, head����
		else if((j==fnum_t)&&((*head+frame-1)==fnum_t)&&(direct==2))	(*head)= fnum_t-frame+1;//Խ�½�, head����
		else if((j==(*head+frame))&&(direct==2))						(*head)++;				//��������һ��
		else if(( (j+1)==*head )&&(direct==1))							(*head)--;				//��������һ��
		else ;
		if(fnum_t == 0)	//����ļ�������0(�ļ�������û���ļ�), ��ɶҲ����ʾ
			return;
		for(u8 i=0;i<frame;i++)
		{
			oled_show_string(2, 13*(i+1), (u8*)filename[i+*head-1],12, j==(i+*head)? 0:1);	//��ѡ�е���Ŀ���з�ɫ��ʾ
			if(i == (fnum_t-1))		//����ļ���̫��, ����ʾ���˾�return
				return;
		}
	}
	else
	{
		oled_show_num_hide0(13, 20, f_res,2,12,0);
		oled_show_string(13, 36, (u8*)"FileList Error.",12,1);
	}
	/*
		//��ȡ�ļ��б�
		if( (f_res==FR_OK)&&(user_key_value==1) )
		{
			user_key_value = 0;//��ȡ�ļ��б��Ĳ���ִֻ��һ��
			
			//oled_clear();
			f_res = ef_scanfiles("0:",&fnum);
			if(f_res==FR_OK)
			{
				oled_show_string(0, 0, cur_path,12,1);//��ʾ ��ǰ·��
				oled_show_num_hide0(108, 0, fnum,2,12,1);//һ�������ļ�
				for(i=0; i<3; i++)
				{
					if(lower_item && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
					{
						lower_item = 0;
						j++;
					}
					else if(upper_item && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
					{
						upper_item = 0;
						j--;
					}
					else ;
					oled_show_string(0, 13*(i+1), filename[i],12,j==(i+1)? 0:1);
				}
			}
			else
				oled_show_num_hide0(13, 0, f_res,2,12,1);
		}
		else;
	*/
}



/**
  * @brief  ����ļ��ĺ�׺��
  * @note	Ŀǰ֧�ֵĺ�׺: txt, bin,
  * @param  f_name: �ļ���(�������벻����·���Ĵ��ļ���)
  * @retval ��׺����Ӧ�ı��, 0 means error
  */
u8 ef_check_extension(char* f_name)
{
	u16 len = strlen(f_name);
	//ʹ��strstr()����".txt", ֮����������һ��"."�Ƿ�ֹmytxt.doc����ʶ��
	if(strstr(f_name+len-6, ".txt"))		return F_TXT;
	//����085664��ʽ��bin�ļ� (��Ϊ10680.binʱ0.96��oled��Ļװ����)
	else if(strstr(f_name+len-6, "64.bin"))	return F_64_BIN;
	//֮����������һ��"."��ϣ���ܿ��ļ���������ֵ�"bin", ��mybin.doc
	else if(strstr(f_name+len-6, ".bin"))	return F_BIN;
	else if(strstr(f_name+len-6, ".fon"))	return F_FON;
	else if(strstr(f_name+len-6, ".mp3"))	return F_MP3;
	else if(strstr(f_name+len-6, ".wav"))	return F_WAV;
	else if(strstr(f_name+len-6, ".jpeg"))	return F_JPEG;
	else if(strstr(f_name+len-6, ".jpg"))	return F_JPG;
	else if(strstr(f_name+len-6, ".png"))	return F_PNG;
	else if(strstr(f_name+len-6, ".bmp"))	return F_BMP;
	else if(strstr(f_name+len-6, ".mp4"))	return F_MP4;
	else if(strstr(f_name+len-6, ".avi"))	return F_AVI;
	else
		return 0;
}




/**
  * @brief  oled��ʾtxt��ʽ�ļ�
  * @note	
  * @param  /name: txt�ļ���(�������벻����·���Ĵ��ļ���)
			/path: txt�ļ�·��
			lib:  �����С(1608, 1206, 0806)
  * @retval None
  */
void ef_multimedia_txt(char* path_t/*f_name*/, u8 lib)
{
	//char path_t[127];
	oled_buffer_clear(0, 0, 127, 63);
	//sprintf(path_t, "%s%s%s", cur_path, "/", f_name);
//	f_res = f_open(&fp, "0:string.txt", FA_READ | FA_OPEN_EXISTING);
	f_res = f_open(&fp, path_t/*f_name*/, FA_READ | FA_OPEN_EXISTING);
	if(f_res==FR_OK)
	{
//		if(fp.fsize>255)	fp.fsize = 255;		//Ŀǰ str[256] �Ŀռ����ֻ��256�ֽ�
		f_res = f_read(&fp, str, fp.fsize, &br);
		
		str[fp.fsize] = '\0';	//���ļ�ĩβǿ�м����ַ���������
		
		if(f_res==FR_OK)
//			{oled_show_num_every0( 0,35,str[0],3,12,0);
//			oled_show_num_every0(30,35,str[1],3,12,0);
//			oled_show_num_every0(60,35,str[2],3,12,0);}
			oled_show_string((lib==16)?0:2, 0, (u8*)str, lib, 1);	//����ʾ1206/0806��Ҫƫ��2��, ��Ϊ21*6=126
		else if(br < fp.fsize)
			oled_show_string(0, 13, (u8*)"error: br < btr.",12,1);
		else
			oled_show_string(0, 13, (u8*)"can't read string.",12,1);
		/* Close the file */
		f_close(&fp);
	}
	else
		oled_show_string(0, 13, (u8*)"can't open txt.",12,0);
//			f_res = f_open(&fp, "0:string.txt", FA_READ | FA_OPEN_EXISTING);
//			if(f_res==FR_OK)
//			{
//				f_res = f_read(&fp, str, fp.fsize, &br);
//				if(f_res==FR_OK)
//					oled_show_string(2, 0, str, 8, 1);//��ʾ1206�ַ���Ҫƫ��2��, ��Ϊ21*6=126
//				else if(br < fp.fsize)
//					oled_show_string(0, 13,"error: br<btr.",12,1);
//				else
//					oled_show_string(0, 13,"can't read string.",12,1);
//				/* Close the file */
//				f_close(&fp);
//			}
//			else
//				oled_show_string(0, 0,"can't open.",12,1);
}




/**
  * @brief  ����bin��Ƶ�ļ� "mix_all_08564.bin"
  * @note	ע������binͼƬ��bin��Ƶ
  * @param  path: bin�ļ�·��
			x,y : ��ʾͼ������Ͻ�����
			frame:��Ƶ����֡��(һ��������ͼƬ)
  * @retval None
  */
void ef_playvideo_bin08564(char* path_t, u8 x, u8 y, u16 frame)
{
	oled_buffer_clear(0, 0, 127, 63);
//	f_res = f_open(&fp, "0:mix_all_08564.bin", FA_READ | FA_OPEN_EXISTING);
	f_res = f_open(&fp, path_t, FA_READ | FA_OPEN_EXISTING);
	if(f_res==FR_OK)
	{
//		if(frame < 1000)	frame = frame;
//		else				frame = 1000;
		for(u16 i=0; i<frame; i++)
		{
//			f_res = f_read(&fp, bin_pic, 340, &br);//���ζ�680���������, 2�ζ�340���ֺܺ�
//			f_res = f_read(&fp, bin_pic+340, 340, &br);
			f_res = f_read(&fp, bin_pic, 680, &br);
			if(f_res==FR_OK)
			{
				//oled_clear();
				delay_ms(21);//30fps = 1/33ms, ����Ҫ���Ƕ�д����ʾ����ʱ
				oled_show_BinImage(x, y, bin_pic, 1);
				oled_refresh();
			}
			else if(br < 340)
			{
				oled_show_string(0, 13, (u8*)"error: br<btr.",12,0);
				break;
			}
			else
			{
				oled_show_string(0, 13, (u8*)"file can't read.",12,0);
				break;
			}
			
			//ȷ����=pause/resume    ���ؼ�=(pause/end)�Ժ�return��FileList
			if(right_key_value)					//����ȷ������ͣ
			{
				right_key_value=0;
				left_key_value=0;
				while(right_key_value == 0)		//�ٴΰ���ȷ�����ͼ���
				{
					if(left_key_value)
					{
						//left_key_value = 0;	//���ؼ�ֵ����0���Ե��º������fps˳��Ҳ�˳���
						return ;
					}
				}
				right_key_value=0;
				left_key_value=0;
			}
		}
		oled_buffer_clear(109, 50, 127, 63);
		oled_show_string(110, 51, (u8*)"end",12,1);	//��ӳ���������½���ʾend
		
		/* Close the file */
		f_close(&fp);
	}
	else
		oled_show_string(0, 0, (u8*)"File path error.",12,1);
	
//	while(left_key_value==0)
//	{
//		if(Trig_oled_FPS)//25Hz
//		{
//			Trig_oled_FPS = 0;
//			oled_refresh();
//		}
//		else;
//	}
}




/**
  * @brief  �ӹ̶�·���򿪶�Ӧ��GBK��������, ���ع̶���С�ĵ���������Ϣ
  * @note	����Ŀ¼�̶�Ϊ"0:GBK_lib/GBKxx.fon", ����Ŀǰ֧��xx=12,16,24
			��ע������������� fast��static����������, ����򿪲�ͬ�ֿ�, ��Ӧ�ø�ֵԭ���Ĳ��ұ���, ����Ӧ��!
  * @param  GbkCode: ����2Byte��GBK����
			lib    : �������ֺŴ�С
			matrix : ���صĵ���������Ϣ
			fast   : =1ʱʹ��CLMT(Cluster Link Map Table)����fast_seek, =0ʱ������
  * @retval rtn	0		if OK,
			rtn f_res	if error,
			rtn 31		if lib_error,
			rtn 30		if f_lseek didn't success.
  */
u8 ef_SD_GetGbkMatrix(u8 GbkCode_H, u8 GbkCode_L, u8 lib, u8* matrix, u8 fast)
{
	/* ��ֹ��ʱ���ֿ�Ḳ��ȫ�ֵ�fp�ȱ���, ������뼴�ü�����fp_t */
	FIL		fp_t;				//�ļ��ṹ��ָ��
	UINT	/*bw_t,*/ br_t;		//��ǰ�ܹ�д��/��ȡ���ֽ���(����f_read(),f_write())
	//FILINFO	fno_t;				//�ļ���Ϣ
	FRESULT	f_res_t;			//File function return code (FRESULT)
	
	DWORD offset;//= unsigned long or uint_32
	if((lib!=12)&&(lib!=16)&&(lib!=24))//����ֺŲ�֧���򷵻�
		return 31;
	switch(lib)
	{
		case 12: f_res_t = f_open(&fp_t,"0:GBK_lib/GBK12.fon", FA_READ | FA_OPEN_EXISTING); break;
		case 16: f_res_t = f_open(&fp_t,"0:GBK_lib/GBK16.fon", FA_READ | FA_OPEN_EXISTING); break;
		case 24: f_res_t = f_open(&fp_t,"0:GBK_lib/GBK24.fon", FA_READ | FA_OPEN_EXISTING); break;
		default: break;
	}
	if(f_res_t==FR_OK)
	{
		u8 csize = (lib/8 + ((lib%8)?1:0)) * lib;	//12->24Byte, 16->32Byte, 24->72Byte
		
		/* GBK ��һ�ֽ�Ϊ0x81~0xFE, �ڶ��ֽ�Ϊ0x40~0x7E �� 0x80~0xFE 
		   �ֿ�����ļ������һ�ֽ�Ϊ����, �ڶ��ֽ�Ϊ���ڵ����
		   ��: ÿ������(��0x7E=126)������190�������(0x3E+1+0x7E+1=0xBE=190)
		   ÿ��������������ÿ�����ֶ�Ӧ���ֽ�����(����ÿ���ֺŵ��ֽ����ǹ̶���)*/
		GbkCode_H -= 0x81;//����
		if(GbkCode_L <= 0x7E)	GbkCode_L -= 0x40;//����0x40,0x7E֮��, ��������λ�������µ�0x00,0x3E֮��
		else					GbkCode_L -= 0x41;//����0x80,0xFE֮��, ��������λ�������µ�0x3F,0xBE֮��
		offset = (GbkCode_H*190 + GbkCode_L) * csize;
		
		if(fast)
		{
			static DWORD clmt[20];					// Cluster link map table buffer
			if(clmt[0]==0)
			{
				f_res_t = f_lseek(&fp_t, offset);		// This is normal seek (fp.cltbl is nulled on file open)
				fp_t.cltbl = clmt;					// Enable fast seek function (fp.cltcl != NULL)
				clmt[0] = 20;						// set the size of array to the first item
				f_res_t = f_lseek(&fp_t, CREATE_LINKMAP);// Creat CLMT
				if(f_res_t == FR_NOT_ENOUGH_CORE)
					//oled_show_string(0, 0,"size_a[] not enough.",12,1);
					return f_res_t;			// rtn (18) if array size is not enough
			}
			else
				fp_t.cltbl = clmt;			// re-set fp.cltbl pointed to clmt (for it was NULL after opend)
			
			f_res_t = f_lseek(&fp_t, offset);			// This is fast seek
			f_res_t = f_read(&fp_t, matrix, csize, &br_t);
		}
		else
		{
			f_res_t = f_lseek(&fp_t, offset);
			if(f_res_t || fp_t.fptr!=offset)	// check if the pointer moved currectly
				return 30;						// rtn 30 if f_lseek failed
			f_res_t = f_read(&fp_t, matrix, csize, &br_t);// normal seek
		}
		
		f_close(&fp_t);
//		switch(lib)
//		{
//			case 12: f_close(&fp_t); break;
//			case 16: f_close(&fp_t); break;
//			case 24: f_close(&fp_t); break;
//			default: break;
//		}
		return 0;
	}
	else
		return f_res_t;
}





/**
  * @brief  ����ȷ���������ϵ���: �����ļ��л���ļ�
  * @note	���ȴ�"0:"����iterate���� �� ɨ�赱ǰ·��
			while(left_key)
			{	����ƶ�(������ʾ)
				��ʾ�ļ��б�
				if ȷ����
				{	malloc �ַ����洢��·��
					if �ļ��� -> ����iterate -> �˳�ĳһ�������� ����ɨ�踸��·��
					else �ļ� -> ȷ�� ��չ�� -> ���ļ� ֱ���˳�
					free �ַ���
				}
			}
  * @param  path: ���ϵ���������·��(�״ε����뵱Ȼ����drv_path����"0:")
  * @retval None
  */
void ef_FileList_iteration(char* path)//�ļ��б�FileList�ĵ���iterate
{
	/*static */u8 j1 = 1;		//��Ŀ��λֵ j1 ���������õ�����ֻ�ܳ�ʼ��һ��, �ʴ˴�static����!
	/*static */u8 delta = 0;	//delta=1����j1--, delta=2����j1++
//	u8	upper_item = 0;
//	u8	lower_item = 0;
	u8 fnum = 0;
	u8 head = 1;
	
	//ɨ���ļ��б�
	f_res = ef_scanfiles(path, &fnum);
	
	while(left_key_value==0)//�����ؼ��˳�
	{
		adc_thrust	= (ADC_Mem[0]+ADC_Mem[5])/2;
		adc_yaw		= (ADC_Mem[1]+ADC_Mem[6])/2;
		adc_pitch	= (ADC_Mem[2]+ADC_Mem[7])/2;
		adc_roll 	= (ADC_Mem[3]+ADC_Mem[8])/2;
		adc_battery = (ADC_Mem[4]+ADC_Mem[9])/2;
		
		if( (adc_pitch>2460)&&(upper_item==0) )			upper_item = 1;	//��ҡ��������
		else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;	//��ҡ��������
		else ;
		if((lower_item==1) && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
		{
			lower_item = 0;
			j1++;
			delta = 2;
			if(j1 > fnum)		j1 = fnum;	//j1ֻ��ȡ 1 2 3 4, ��j1��ѭ��
			else ;
		}
		else if((upper_item==1) && (adc_pitch>2001)&&(adc_pitch<2099))//�������ѡ����ҡ�˻�����
		{
			upper_item = 0;
			j1--;
			delta = 1;
			if(j1 < 1)			j1 = 1;		//��ֹj1=-1, ���趨jֻ��ȡ 1 2 3 4, ��j1��ѭ��
			else ;
		}
		else ;
		
		//��ʾ�ļ��б�
		//��ǰ·��path,�ļ�����fnum,��Ļ�ߴ�ֻ����ʾ4���ļ���,����j1����ɫ,�����ƶ�����delta, ��һ����ʾ���ļ���head
		ef_display_filelist(path, fnum, 4, j1, delta, &head);
		delta = 0;
		
		if(Trig_oled_FPS)//25Hz
		{
			Trig_oled_FPS = 0;
			oled_refresh();
		}
		else ;
		
		if(right_key_value)
		{
			left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;

			char* temp_path = (char*)malloc(sizeof(char)*120);//Ϊ��ǰ·�������ڴ�

			sprintf(temp_path, "%s%s%s", path, "/", filename[j1-1]);//ƴ��·��
			
			//����ȷ�������ж��Ƿ����ļ���
			if(ef_checkfile(temp_path) & AM_DIR)	//if it is a DIR
			{
				//����� �ļ��� ����� ����
				ef_FileList_iteration(temp_path);
				
				//�ٴ�ɨ���ļ��б�(ע������ɨ�����ԭʼ·��)
				f_res = ef_scanfiles(path, &fnum);
			}
			else									//if it is a FIL
			{
				//����� �ļ� ��ȷ��������չ��
				u8 t = ef_check_extension(filename[j1-1]);//������ temp_path Ҳ����
				
				if(t == F_TXT)
				{
					/* �ڶ��������Ǽ��±����ֺ�(Ӣ��0806/1206/1608, ����12/16/24) */
					ef_multimedia_txt(temp_path, 12);//filename[j1-1], �ǵ��޸�display_txt()�����·��(���Ƿ�Ϊ���ļ���������·��)
					while(left_key_value==0)
					{
						if(Trig_oled_FPS)//25Hz
						{
							Trig_oled_FPS = 0;
							oled_refresh();
						}
						else;
					}
					oled_buffer_clear(0,0,127,63);
					left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
				}
				else if(t == F_64_BIN)	//׼������ *64.bin �ļ�, ע������ͼƬ����Ƶ�ļ�!
				{
					if(fno.fsize == 68000)
						ef_playvideo_bin08564(temp_path, 21, 0, 99);	//Ϊ�˼���mix_100_08564.bin
					else
						ef_playvideo_bin08564(temp_path, 21, 0, fno.fsize/680);	//6570֡
					while(left_key_value==0)
					{
						if(Trig_oled_FPS)//25Hz
						{
							Trig_oled_FPS = 0;
							oled_refresh();
						}
						else;
					}
					oled_buffer_clear(0,0,127,63);
					left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
				}
				else if(t == F_BIN)	//׼������ *.bin �ļ�, ע������ͼƬ����Ƶ�ļ�!
				{
//					ef_display_bin(filename[j1-1], 8);	//�������ļ���С������ͼƬ����Ƶ
					oled_buffer_clear(9, 15, 117, 54);
					oled_draw_rectangle(11, 17, 115, 52, 1);
					oled_show_string(13, 19, (u8*)"   ! caution !   ",12,1);
					oled_show_string(13, 40, (u8*)"Unfinished Part..",12,1);
					while(left_key_value==0)
					{
						if(Trig_oled_FPS)//25Hz
						{
							Trig_oled_FPS = 0;
							oled_refresh();
						}
						else;
					}
					oled_buffer_clear(0,0,127,63);
					left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
				}
				else if(t == F_FON)	//׼������ *.fon �ļ�, ��Get_Matrix()����ķ�ʽ����ֹ�����ֿ�!
				{
					oled_buffer_clear(9, 15, 117, 54);
					oled_draw_rectangle(11, 17, 115, 52, 1);
					oled_show_string(13, 19, (u8*)"   !  ERROR  !   ",12,1);
					oled_show_string(13, 40, (u8*)"forbidden access!",12,1);
					while(left_key_value==0)
					{
						if(Trig_oled_FPS)//25Hz
						{
							Trig_oled_FPS = 0;
							oled_refresh();
						}
						else;
					}
					oled_buffer_clear(0,0,127,63);
					left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
				}
				else
				{
					oled_buffer_clear(9, 15, 117, 54);
					oled_draw_rectangle(11, 17, 115, 52, 1);
					oled_show_string(13, 19, (u8*)"   ! caution !   ",12,1);
					oled_show_string(13, 40, (u8*)"Unsupport File...",12,1);
					while(left_key_value==0)
					{
						if(Trig_oled_FPS)//25Hz
						{
							Trig_oled_FPS = 0;
							oled_refresh();
						}
						else;
					}
					oled_buffer_clear(0,0,127,63);
					left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
				}
			}
			
			//�ͷ��ڴ�
			free(temp_path);
		}
		else ;
	}
	left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
}


