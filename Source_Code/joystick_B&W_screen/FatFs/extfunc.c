#include "extfunc.h"
//#include "ff.h"
#include "sdio_sdcard.h"
#include "oled.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
/*【说明】：
/  该c文件定义了基于FatFs模块的扩展函数, 只保留核心函数则无需调用oled.h		  *
/  最核心的函数只有三个：ef_getcapacity(), ef_scanfiles(), ef_checkfile()	  *
/  后面的附加函数都调用了核心函数, 他们主要用于简化 main.c , 移植时可以删除   */
/**
  ******************************************************************************
  *                             定义变量
  ******************************************************************************
  */
extern SD_CardInfo SDCardInfo;

FATFS 	fs;					//FatFs文件系统结构体指针
FIL		fp;					//文件结构体指针
UINT	bw,br;				//当前总共写入/读取的字节数(用于f_read(),f_write())
FILINFO	fno;				//文件信息
FRESULT	f_res;				//File function return code (FRESULT)
char	filename[15][127];	//可以改成malloc()
char 	cur_path[10]="0:";	//保存当前所在的路径 either "0:" or "0:/" is OK
u32 	tot_size=0, fre_size=0, sd_size=0;
//u8		fnum=0;				//路径下的文件(夹)个数

//因为汉字编码大于0x80=128>ascii, 所以必须为str[]应该定义成 u8* 格式
extern char	str[256];		//存储从SD卡的txt文件读出的字符串
extern char bin_pic[680];	//存储从SD卡的bin文件读出的一帧图像

extern u16	ADC_Mem[10];
extern u8	left_key_value;
extern u8 	right_key_value;	//用于取反
extern u8	user1_key_value;
extern u8	user2_key_value;
extern u8	upper_item;
extern u8	lower_item;
extern u16	adc_thrust;
extern u16	adc_yaw;
extern u16	adc_pitch;			//前进后退
extern u16	adc_roll;			//左右侧移
extern u16	adc_battery;
extern u8	Trig_oled_FPS;		// 25Hz


/**
  ******************************************************************************************************
  *                                         自定义扩展函数 (核心)
  ******************************************************************************************************
  */
/**
  * @brief  得到磁盘容量
  * @param  drv_path: 磁盘路径("0:"或"1:")
  *			* tot/fre_size: FatFs容量(KB)
			* sd_size: SD卡的全部物理容量
  * @retval f_res: 文件系统返回值FRESULT
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
		tot_sect = (fs1->n_fatent - 2) * fs1->csize;	//总扇区数
		fre_sect = fre_clust * fs1->csize;			//空闲扇区数
		*tot_size = tot_sect>>1;	//单位是KB
		*fre_size = fre_sect>>1;	//单位是KB
#if	_MAX_SS != _MIN_SS
		*tot_size *= fs1->ssize * 4;	//若MAX_SS配置为4096, 则等于4KB
		*fre_size *= fs1->ssize * 4;	//空闲KB千字节数也扩大4倍
#endif
		*sd_size = SDCardInfo.CardCapacity>>10;//单位是KB
	}
	return f_res;
}


/**
  * @brief  扫描当前目录下的所有文件名
  * @param  *path: 扫描路径
  *			*num:  该路径下的项目数量(包括文件夹和文件)
  * @retval f_res: 文件系统返回值FRESULT
  */
FRESULT ef_scanfiles(char* path, u8* num)
{
	FRESULT	f_res;
	DIR dir;		//目录结构体指针
	char* fn;
	u8 i = 0;		//路径下的文件个数
	//u8 len = 0;		//路径名的长度, 方便叠加路径名
	
#if _USE_LFN 
  /* 长文件名支持 */
  /* 简体中文一个汉字需要2个字节 */
  static char lfn[_MAX_LFN*2 + 1]; 	
  fno.lfname = lfn; 
  fno.lfsize = sizeof(lfn); 
#endif 
	
	f_res = f_opendir(&dir, (const TCHAR*)path);//打开目录
	if(f_res==FR_OK)
	{
		//len = strlen(path);
//		sprintf(cur_path, "%s%s", path, "/");//在路径的结尾追加一个slash
		//strcpy(cur_path, path);
		//strcat(cur_path, "/");//在路径的结尾追加一个slash
		while(1)
		{
			f_res = f_readdir(&dir, &fno);		//读当前目录, 再次读取会自动读下一个文件
			if(f_res!=FR_OK || fno.fname[0]==0)	//读取出错/读到末尾(返回空)时结束
			//{
				//if(i == 0)	strcpy(filename[0], "\0");	//如果一个文件也读不到, 至少应该留下一个\0
				break;
			//}
//			if(fno.lfname[0]=='.')	continue;	//忽略文件后缀名
#if _USE_LFN 
			fn = *fno.lfname ? fno.lfname : fno.fname;
#else
			fn = fno.fname;
#endif
			if(fno.fattrib & AM_HID)	//It is a hidden Dir/File
				continue;				//跳过, 不计入总数num
//			if(fno.fattrib & AM_DIR)	//It is a directory
//				strcat(filename[i],"+");//在文件夹名字头部加入"+"
//			else						//It is a file
//				strcat(filename[i]," ");//在文件的名字头部加入" "
//			strcat(filename[i], fn);
			strncpy(filename[i], fn, 126);	//filename总长127, 防止内存溢出仅填充126
			filename[i][126]='\0';			//在末尾第127字节插入\0保证至少一个结束符
			i++;
		}
		*num = i;
		f_closedir(&dir);	//关闭当前目录
	}
	return f_res;
}



/**
  * @brief  获取文件(夹)属性
  * @note	注意此函数能够保存文件的全部属性, 不过返回值仅仅是标识!
  * @param  *path: 文件(夹)的路径(要包含文件名和后缀)
  * @retval file_attribute: 文件(夹)属性标识
  */
u8 ef_checkfile(char* path)
{
	FRESULT f_res = f_stat((const TCHAR*)path, &fno);
	if(f_res==0)
	{
/*		printf(">>文件大小: %ld(Byte)\n", fno.fsize);
		printf(">>修改时间: %u/%02u/%02u, %02u:%02u\n",(fno.fdate >> 9) + 1980,
			fno.fdate >> 5 & 15, fno.fdate & 31,fno.ftime >> 11, fno.ftime >> 5 & 63);
		printf(">>属性: %c%c%c%c%c\n\n",
           (fno.fattrib & AM_DIR) ? 'D' : '-',      //Director(文件夹/目录)
           (fno.fattrib & AM_RDO) ? 'R' : '-',      //Read-Only
           (fno.fattrib & AM_HID) ? 'H' : '-',      //Hidden
           (fno.fattrib & AM_SYS) ? 'S' : '-',      //System
           (fno.fattrib & AM_ARC) ? 'A' : '-');     //Archive(档案文件)
		*/
		return fno.fattrib;
	}
	else
		return 0;	//0代表错误
}




/**
  ******************************************************************************************************
  *                                        自定义扩展函数 (附加)
  ******************************************************************************************************
  */
/**
  * @brief  oled显示SD卡的各项容量
  * @note	此函数禁止循环调用(不要重复读写SD卡)
  * @param  drv_path: 卷标号
  * @retval None
  */
void ef_display_SDcapcity(char* drv_path)
{
//	if(SD_Init()==SD_OK)
//		oled_show_string(0, 52,"SD_OK.",12,1);
//	else
//		oled_show_num_hide0(0, 52, SD_Init(),2,12,1);
//	oled_show_num_hide0(42, 52, SDCardInfo.CardCapacity>>10,11,12,1);//SD卡的物理容量
//	oled_show_num_hide0(84,  0, SDCardInfo.CardType,2,12,1);
	
	if(f_res==FR_OK)
	{
		oled_show_string(0, 0, (u8*)"FatFs ready. ",12,1);
		f_res = ef_getcapacity(drv_path, &tot_size, &fre_size, &sd_size);	//ef_getcapacity("0:", &tot_size, &fre_size, &sd_size);
		if(f_res==FR_OK)
		{
			oled_show_string(0, 18, (u8*)"SDcap:        KB",12,1);
			oled_show_num_hide0(36, 18, sd_size>>0, 7,12,0);//高亮显示SD卡的物理容量
			oled_show_string(0, 33, (u8*)"total:        KB",12,1);
			oled_show_num_hide0(36, 33, tot_size>>0,7,12,1);
			oled_show_string(0, 48, (u8*)"spare:        KB",12,1);
			oled_show_num_hide0(36, 48, fre_size>>0,7,12,1);
		}
		else
			oled_show_num_hide0(0, 33, f_res,2,12,0);	//高亮显示错误号
	}
	else
		oled_show_num_hide0(0, 0, f_res,2,12,0);		//高亮显示错误号
	//oled_refresh();
}



/**
  * @brief  oled显示当前目录下的文件
  * @note	在此函数之前应当使用 ef_scanfile() 函数得到文件数fnum
  * @param  path: 当前路径
			fnum: 当前路径下的文件数
			frame:一个屏幕每次能够显示的文件个数
			j = j1 为当前选中的索引号
			direct = delta 代表当前是期望向下还是向上(有可能越上下界)
  * @retval None
  */
void ef_display_filelist(char* path, u8 fnum_t, u8 frame, u8 j, u8 direct, u8* head)
{
//*** 注意每一页的文件列表只读取一次, 显示次数不限, 反正就是不要发疯一样的访问SD卡 ***
//	static u8 head = 1;
	if(f_res==FR_OK)
	{
		oled_buffer_clear(0, 0, 127, 63);
		oled_show_string(0, 0, (u8*)path,12,1);		//显示当前路径
		oled_draw_line(0, 13, 0, 63, 1);			//显示左侧铅垂线
		oled_show_num_hide0(110, 0, fnum_t,3,12,1);	//一共几个文件
		if(fnum_t <= frame)												(*head)= 1;				//总数太小, head保持1
		else if((j==1)&&(*head==1)&&(direct==1))						(*head)= 1;				//越上界, head不变
		else if((j==fnum_t)&&((*head+frame-1)==fnum_t)&&(direct==2))	(*head)= fnum_t-frame+1;//越下界, head不变
		else if((j==(*head+frame))&&(direct==2))						(*head)++;				//整体下移一行
		else if(( (j+1)==*head )&&(direct==1))							(*head)--;				//整体上移一行
		else ;
		if(fnum_t == 0)	//如果文件个数是0(文件夹里面没有文件), 则啥也不显示
			return;
		for(u8 i=0;i<frame;i++)
		{
			oled_show_string(2, 13*(i+1), (u8*)filename[i+*head-1],12, j==(i+*head)? 0:1);	//对选中的条目进行反色显示
			if(i == (fnum_t-1))		//如果文件数太少, 则显示完了就return
				return;
		}
	}
	else
	{
		oled_show_num_hide0(13, 20, f_res,2,12,0);
		oled_show_string(13, 36, (u8*)"FileList Error.",12,1);
	}
	/*
		//读取文件列表
		if( (f_res==FR_OK)&&(user_key_value==1) )
		{
			user_key_value = 0;//读取文件列表的操作只执行一次
			
			//oled_clear();
			f_res = ef_scanfiles("0:",&fnum);
			if(f_res==FR_OK)
			{
				oled_show_string(0, 0, cur_path,12,1);//显示 当前路径
				oled_show_num_hide0(108, 0, fnum,2,12,1);//一共几个文件
				for(i=0; i<3; i++)
				{
					if(lower_item && (adc_pitch>2001)&&(adc_pitch<2099))//如果向下选择且摇杆回中了
					{
						lower_item = 0;
						j++;
					}
					else if(upper_item && (adc_pitch>2001)&&(adc_pitch<2099))//如果向上选择且摇杆回中了
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
  * @brief  检查文件的后缀名
  * @note	目前支持的后缀: txt, bin,
  * @param  f_name: 文件名(建议输入不包括路径的纯文件名)
  * @retval 后缀名对应的标号, 0 means error
  */
u8 ef_check_extension(char* f_name)
{
	u16 len = strlen(f_name);
	//使用strstr()搜索".txt", 之所以所检索一个"."是防止mytxt.doc被误识别
	if(strstr(f_name+len-6, ".txt"))		return F_TXT;
	//搜索085664格式的bin文件 (因为10680.bin时0.96寸oled屏幕装不下)
	else if(strstr(f_name+len-6, "64.bin"))	return F_64_BIN;
	//之所以所检索一个"."是希望避开文件本名里出现的"bin", 如mybin.doc
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
  * @brief  oled显示txt格式文件
  * @note	
  * @param  /name: txt文件名(必须输入不包括路径的纯文件名)
			/path: txt文件路径
			lib:  字体大小(1608, 1206, 0806)
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
//		if(fp.fsize>255)	fp.fsize = 255;		//目前 str[256] 的空间最多只能256字节
		f_res = f_read(&fp, str, fp.fsize, &br);
		
		str[fp.fsize] = '\0';	//在文件末尾强行加入字符串结束符
		
		if(f_res==FR_OK)
//			{oled_show_num_every0( 0,35,str[0],3,12,0);
//			oled_show_num_every0(30,35,str[1],3,12,0);
//			oled_show_num_every0(60,35,str[2],3,12,0);}
			oled_show_string((lib==16)?0:2, 0, (u8*)str, lib, 1);	//若显示1206/0806则要偏移2列, 因为21*6=126
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
//					oled_show_string(2, 0, str, 8, 1);//显示1206字符串要偏移2列, 因为21*6=126
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
  * @brief  播放bin视频文件 "mix_all_08564.bin"
  * @note	注意区分bin图片和bin视频
  * @param  path: bin文件路径
			x,y : 显示图像的左上角坐标
			frame:视频的总帧数(一共多少张图片)
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
//			f_res = f_read(&fp, bin_pic, 340, &br);//单次读680会出现乱码, 2次读340表现很好
//			f_res = f_read(&fp, bin_pic+340, 340, &br);
			f_res = f_read(&fp, bin_pic, 680, &br);
			if(f_res==FR_OK)
			{
				//oled_clear();
				delay_ms(21);//30fps = 1/33ms, 但还要考虑读写及显示的用时
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
			
			//确定键=pause/resume    返回键=(pause/end)以后return到FileList
			if(right_key_value)					//按下确定就暂停
			{
				right_key_value=0;
				left_key_value=0;
				while(right_key_value == 0)		//再次按下确定键就继续
				{
					if(left_key_value)
					{
						//left_key_value = 0;	//返回键值不清0可以导致函数外的fps顺便也退出掉
						return ;
					}
				}
				right_key_value=0;
				left_key_value=0;
			}
		}
		oled_buffer_clear(109, 50, 127, 63);
		oled_show_string(110, 51, (u8*)"end",12,1);	//放映结束在右下角提示end
		
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
  * @brief  从固定路径打开对应的GBK字体点阵库, 返回固定大小的点阵数组信息
  * @note	字体目录固定为"0:GBK_lib/GBKxx.fon", 字体目前支持xx=12,16,24
			【注】这个函数里面 fast的static数组有问题, 如果打开不同字库, 不应该赋值原来的查找表的, 不对应啊!
  * @param  GbkCode: 输入2Byte的GBK内码
			lib    : 期望的字号大小
			matrix : 返回的点阵数组信息
			fast   : =1时使用CLMT(Cluster Link Map Table)激活fast_seek, =0时不激活
  * @retval rtn	0		if OK,
			rtn f_res	if error,
			rtn 31		if lib_error,
			rtn 30		if f_lseek didn't success.
  */
u8 ef_SD_GetGbkMatrix(u8 GbkCode_H, u8 GbkCode_L, u8 lib, u8* matrix, u8 fast)
{
	/* 防止暂时打开字库会覆盖全局的fp等变量, 这里必须即用即定义fp_t */
	FIL		fp_t;				//文件结构体指针
	UINT	/*bw_t,*/ br_t;		//当前总共写入/读取的字节数(用于f_read(),f_write())
	//FILINFO	fno_t;				//文件信息
	FRESULT	f_res_t;			//File function return code (FRESULT)
	
	DWORD offset;//= unsigned long or uint_32
	if((lib!=12)&&(lib!=16)&&(lib!=24))//如果字号不支持则返回
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
		
		/* GBK 第一字节为0x81~0xFE, 第二字节为0x40~0x7E 和 0x80~0xFE 
		   字库点阵文件定义第一字节为区号, 第二字节为区内点阵号
		   故: 每个区号(共0x7E=126)里面有190个点阵号(0x3E+1+0x7E+1=0xBE=190)
		   每个点阵号里面就是每个汉字对应的字节数据(对于每种字号的字节量是固定的)*/
		GbkCode_H -= 0x81;//区号
		if(GbkCode_L <= 0x7E)	GbkCode_L -= 0x40;//介于0x40,0x7E之间, 点阵数据位于区号下的0x00,0x3E之间
		else					GbkCode_L -= 0x41;//介于0x80,0xFE之间, 点阵数据位于区号下的0x3F,0xBE之间
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
  * @brief  按下确定键并不断迭代: 进入文件夹或打开文件
  * @note	首先从"0:"进入iterate函数 并 扫描当前路径
			while(left_key)
			{	光标移动(反白显示)
				显示文件列表
				if 确定键
				{	malloc 字符串存储总路径
					if 文件夹 -> 迭代iterate -> 退出某一个迭代就 重新扫描父级路径
					else 文件 -> 确定 扩展名 -> 打开文件 直至退出
					free 字符串
				}
			}
  * @param  path: 不断迭代的输入路径(首次的输入当然就是drv_path比如"0:")
  * @retval None
  */
void ef_FileList_iteration(char* path)//文件列表FileList的迭代iterate
{
	/*static */u8 j1 = 1;		//条目定位值 j1 仅在下面用到但是只能初始化一次, 故此处static声明!
	/*static */u8 delta = 0;	//delta=1代表j1--, delta=2代表j1++
//	u8	upper_item = 0;
//	u8	lower_item = 0;
	u8 fnum = 0;
	u8 head = 1;
	
	//扫描文件列表
	f_res = ef_scanfiles(path, &fnum);
	
	while(left_key_value==0)//按返回键退出
	{
		adc_thrust	= (ADC_Mem[0]+ADC_Mem[5])/2;
		adc_yaw		= (ADC_Mem[1]+ADC_Mem[6])/2;
		adc_pitch	= (ADC_Mem[2]+ADC_Mem[7])/2;
		adc_roll 	= (ADC_Mem[3]+ADC_Mem[8])/2;
		adc_battery = (ADC_Mem[4]+ADC_Mem[9])/2;
		
		if( (adc_pitch>2460)&&(upper_item==0) )			upper_item = 1;	//右摇杆向上推
		else if( (adc_pitch<1100)&&(lower_item==0) )	lower_item = 1;	//右摇杆向下推
		else ;
		if((lower_item==1) && (adc_pitch>2001)&&(adc_pitch<2099))//如果向下选择且摇杆回中了
		{
			lower_item = 0;
			j1++;
			delta = 2;
			if(j1 > fnum)		j1 = fnum;	//j1只能取 1 2 3 4, 且j1不循环
			else ;
		}
		else if((upper_item==1) && (adc_pitch>2001)&&(adc_pitch<2099))//如果向上选择且摇杆回中了
		{
			upper_item = 0;
			j1--;
			delta = 1;
			if(j1 < 1)			j1 = 1;		//防止j1=-1, 故设定j只能取 1 2 3 4, 且j1不循环
			else ;
		}
		else ;
		
		//显示文件列表
		//当前路径path,文件个数fnum,屏幕尺寸只许显示4行文件名,将第j1个反色,期望移动方向delta, 第一个显示的文件号head
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

			char* temp_path = (char*)malloc(sizeof(char)*120);//为当前路径分配内存

			sprintf(temp_path, "%s%s%s", path, "/", filename[j1-1]);//拼接路径
			
			//按下确定键后判断是否是文件夹
			if(ef_checkfile(temp_path) & AM_DIR)	//if it is a DIR
			{
				//如果是 文件夹 则继续 迭代
				ef_FileList_iteration(temp_path);
				
				//再次扫描文件列表(注意这里扫描的是原始路径)
				f_res = ef_scanfiles(path, &fnum);
			}
			else									//if it is a FIL
			{
				//如果是 文件 则确定它的扩展名
				u8 t = ef_check_extension(filename[j1-1]);//或输入 temp_path 也可以
				
				if(t == F_TXT)
				{
					/* 第二个参数是记事本的字号(英文0806/1206/1608, 中文12/16/24) */
					ef_multimedia_txt(temp_path, 12);//filename[j1-1], 记得修改display_txt()的入口路径(即是否为纯文件名还是总路径)
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
				else if(t == F_64_BIN)	//准备启动 *64.bin 文件, 注意区分图片和视频文件!
				{
					if(fno.fsize == 68000)
						ef_playvideo_bin08564(temp_path, 21, 0, 99);	//为了兼容mix_100_08564.bin
					else
						ef_playvideo_bin08564(temp_path, 21, 0, fno.fsize/680);	//6570帧
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
				else if(t == F_BIN)	//准备启动 *.bin 文件, 注意区分图片和视频文件!
				{
//					ef_display_bin(filename[j1-1], 8);	//可以用文件大小来区分图片和视频
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
				else if(t == F_FON)	//准备启动 *.fon 文件, 但Get_Matrix()以外的方式都禁止启动字库!
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
			
			//释放内存
			free(temp_path);
		}
		else ;
	}
	left_key_value=0; right_key_value=0; user1_key_value=0; user2_key_value=0;
}



