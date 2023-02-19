#include "delay.h"
#include "oled.h"
#include "oledfont.h"
#include "spi.h"
#include "dma.h"
#include <math.h>
#include "extfunc.h"

///////////////////////////////////////////////////////////////////////////Plexer//
// 1.本程序使用实际像素点坐标, column(0~127)映射为X轴, row(0-63)映射为Y轴
// 2.考虑到显示器的行列号, (0,0)位于左上角, 即X轴正向指右, Y轴正向指下
// 3.因为oled_ssd1306只能页写, 一次要写一列8bit = 1Byte
//   为了防止画线时线条的边缘遮盖之前的痕迹, 本程序引入"数据缓冲包"
// 4."缓冲包"包括单次刷新全屏的一帧数据, 缓存完成再一并写入GRAM来显示
// 5.文件中设置了专门的oled_refresh()函数, 以传送buffer的数据到GRAM中
// 6.所有函数均只能修改buffer数据, 即使清零函数也只是清空buffer而已
// 7.这种缓冲帧数据的做法导致无需清屏, 只要快速刷新每一帧(类似fps)就行
//////////////////////////////////////////////////////////////////////////2020/4/19
//OLED的显存GRAM格式如下:
// [0] 0 1 2 3 ... 127
// [1] 0 1 2 3 ... 127
// [2] 0 1 2 3 ... 127
// [3] 0 1 2 3 ... 127
// [4] 0 1 2 3 ... 127
// [5] 0 1 2 3 ... 127
// [6] 0 1 2 3 ... 127
// [7] 0 1 2 3 ... 127

//把fram_buffer送入GRAM的过程甚至可以启用spi的DMA方式, 那样更快哦
u8 Frame_Buffer[128][8];//fram_buffer中每一项为8bit,一共128*8个8bit


//原子的板子上的PA.234分别是Flash,SDcard,oled的CS端, PC.4是NRF的CS端.
//暂定OLED_CS(PB0),	OLED_RST(PC4), OLED_DC(PC5), 而spi1的另三根线会在SPI1_Init()中得到初始化
void oled_spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
#if DMA_MODE
	DMA_InitTypeDef  DMA_InitStructure;		//DMA1 Structure
#endif	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;	//RST(PC4), DC(PC5)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//影响功耗
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);			//拉高
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//CS(PB0)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//影响功耗
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);						//拉高

#if DMA_MODE	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);			//使能DMA传输
	DMA_DeInit(DMA1_Channel3);									//重置DMA1的通道3(SPI1_TX)
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI1->DR;	//DMA外设基地址 -> SPI1
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&Frame_Buffer;	//DMA内存基地址 -> FrameBuffer
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;			//数据传输方向为数组至外设(SPI)
	DMA_InitStructure.DMA_BufferSize = 128;						//DMA通道的DMA缓存的大小为 128 Byte
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//外设地址寄存器不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//内存地址寄存器自增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;				//工作在普通模式,不循环采集
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;			//DMA通道3的优先级为 较高级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;				//DMA通道3 不是内存到内存传输
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	//DMA_Cmd(DMA1_Channel3, ENABLE);//在oleda_refresh()中启动DMA, 在中断Handler()中禁止DMA
#endif

	SPI1_Init();//初始化SPI1,(函数内配置了 Baud = 4.5M )
}

//向SSD1306写入一个字节, cmd=0表示命令;  /1表示数据;
void oled_write_Byte(u8 dat,u8 cmd)
{
	OLED_DC = cmd; 			//选择 data/cmd
	OLED_CS = 0;		 	//打开spi片选
	SPI1_ReadWriteByte(dat);//spi发送
	OLED_CS=1;		  		//关闭spi片选
	OLED_DC=1;   	  		//默认选择 data
}
	  	  
//开启OLED显示    
void oled_display_on(void)
{
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC命令
	oled_write_Byte(0x14,OLED_CMD);  //DCDC ON
	oled_write_Byte(0xAF,OLED_CMD);  //DISPLAY ON
}

//关闭OLED显示     
void oled_display_off(void)
{
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC命令
	oled_write_Byte(0x10,OLED_CMD);  //DCDC OFF
	oled_write_Byte(0xAE,OLED_CMD);  //DISPLAY OFF
}

//仅将buffer的数据清零(对屏幕暂时没影响,除非完成刷新)
/*
void buffer_clear(void)
{
	u8 x, y;		    
	for(y=0;y<8;y++)
		for(x=0;x<128;x++)
			Frame_Buffer[x][y] = 0x00;
}
*/
//清屏区域必须是矩形, 且x0<x1,y0<y1
//清0区域是包含坐标点的
void oled_buffer_clear(u8 x0, u8 y0, u8 x1, u8 y1)
{
	u8 page0 = y0/8, offset0 = y0%8;
	u8 page1 = y1/8, offset1 = y1%8;
	u8 temp0 = 0xFF << offset0;			//从y0偏移处往下都应清0
	u8 temp1 = 0xFF << (offset1+1);		//从y1偏移处往上都应清0
	u8 x, y;		    

	if(page0 == page1)					//如果清0区域上下处在相同page里
	{
		if(offset0 == offset1)			//如果连offset也一样
		{
			temp0 = 0x01 << offset0;
			for(x=x0; x<x1+1; x++)		//相当于画一条白线
				Frame_Buffer[x][page0] &= ~temp0;
		}
		else							//若在相同page内清除某区域
		{
			temp0 = ~temp0 + temp1;		//求出相夹的区域
			for(x=x0; x<x1+1; x++)		//相当于画一条白色粗线
				Frame_Buffer[x][page0] &= ~temp0;
		}
	}
	else								//如果区域不只在一个page以内
	{
		for(x=x0; x<x1+1; x++)			//先清除page0里的相关区域
			Frame_Buffer[x][page0] &= ~temp0;
		y = page0 + 1;
		while(y < page1)
		{
			for(x=x0; x<x1+1; x++)
				Frame_Buffer[x][y] = 0x00;	//所夹的page一律填充为0
			y++;
		}
		for(x=x0; x<x1+1; x++)			//最后再清除page1里的相关区域
			Frame_Buffer[x][page1] &= temp1;
	}
}

//将buffer的数据送入GRAM 以刷新一帧屏幕图案
void oled_refresh(void)
{
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		oled_write_Byte(0xb0+i,OLED_CMD);    //设置页地址(0~7)
		oled_write_Byte(0x00,OLED_CMD);      //设置列 低地址
		oled_write_Byte(0x10,OLED_CMD);      //设置列 高地址

#if DMA_MODE		
		DMA_ClearFlag(DMA1_FLAG_TC3);
		DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);//开启DMA_TC中断标志
		
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		/*!< DMA1 Channel3 enable */
		DMA_Cmd(DMA1_Channel3, ENABLE);//在oleda_refresh()中启动DMA, 在中断Handler()中禁止DMA
		
		OLED_DC = OLED_DATA;//选择data/cmd
		OLED_CS = 0;
		for(n=0;n<128;n++)
		{
			//OLED_CS = 0;
			SPI1_ReadWriteByte(Frame_Buffer[n][i]);
			//OLED_CS=1;
		}
		OLED_CS=1;
#else
		for(n=0;n<128;n++)
			oled_write_Byte(Frame_Buffer[n][i],OLED_DATA);//LSBit实际在最上面一横排的像素
#endif
	}
}


//清屏
void oled_clear(void)
{
	oled_buffer_clear(0, 0, 127, 63);//默认清0全屏
	oled_refresh();
}

//描点, x(0~127),y(0~63)是像素点坐标,注意(0,0)是在 左上角
//mode:1 (在GRAM中)点亮像素点    0: (在GRAM中)熄灭像素点
void oled_draw_point(u8 x, u8 y, u8 mode)
{
	u8 page, offset;
	u8 temp=0;
	if(x>127 || y>63) return;	//超出范围
	page = y/8;					//所在的页
	offset = y%8;				//页内偏移
	temp = 0x01 << offset;
	
	//因为仅对buffer操作, 并不需要locate()函数选定坐标!
	if(mode)	Frame_Buffer[x][page] |= temp;
	else		Frame_Buffer[x][page] &= ~temp;
}

//在任意坐标显示字符,lib支持0806,1206,1608
//x(0~127),y(0~63)     /mode:0,反白显示(高亮底纹)   1,正常显示
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u8 mode)
{      			    
	u8 temp,i,k;
	u8 y0 = y, x0 = x;	//记录原始y和x
	chr = chr - ' ';	//得到偏移后的值(所在的行数)
	if(lib==8)			//0806
	{
		for(i=0; i<6; i++)
		{
			if((x>=128)||(y>=64))	continue;		//仅显示到没越界的部分(用来兼容逗号和句号)如果右侧越界, 则跳过
				
			temp = ascii_0806[chr][i];
			for(k=0; k<8; k++)				//每次描8个点
			{
				if(temp&0x01)	oled_draw_point(x,y,mode);//判断temp的末位是否为1
				else 			oled_draw_point(x,y,!mode);
				temp >>= 1;
				y++;			//准备操作下方的一个元素点
			}
			y = y0; x++;					//继续描右侧一列的点
		}
	}
	else if(lib==12 || lib==16)	//1206,1608
	{
		u8 csize=( lib/8+((lib%8)?1:0) ) * (lib/2);	//一个字符占的字节数,1206为12,1608为16
		for(i=0;i < csize;i++)
		{
			if((x>128)||(y>=64))	continue;		//仅显示到没越界的部分(用来兼容逗号和句号)如果右侧越界, 则跳过
			if(lib==12)		temp = ascii_1206[chr][i];
			else			temp = ascii_1608[chr][i];
			if(i < csize/2)		//1206首次 描8点; 1608每次描8点
			{
				for(k=0; k<8; k++)
				{
					if(temp&0x01)	oled_draw_point(x,y,mode);//判断temp的末位是否为1
					else			oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//准备操作下方的一个元素点
				}
				y = y0; x++;						//继续描右侧一列的点
			}
			else				//1206第二次 描4点; 1608每次描8点
			{
				y = y0 + 8;
				x = x0 + i - csize/2;	//定位到开始第二次扫描的新坐标
				for(k=0; k<8; k++)
				{
					if(temp&0x01)	oled_draw_point(x,y,mode);//判断temp的末位是否为1
					else			oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//准备操作下方的一个元素点
					if((y-y0)==lib)					//如果1206描到第12个点,则即刻换成下一列
					{
						y = y0+8;
						x++;
						break;						//打断1206的描点,换成下一列继续描点
					}
				}
			}
		}
	}
	else
		return;//无对应的字符集
}

//m^n次幂函数,本文件中内部调用
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)
		result *= m;    
	return result;
}

//显示一组数字(高位的0不显示)
//x,y:坐标	  /len:几位数   /lib:字体   /mode:0,反白显示  1,正常显示			  
void oled_show_num_hide0(u8 x, u8 y, u32 num, u8 len, u8 lib, u8 mode)
{         	
	u8 temp;
	u8 enshow=0;
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//从左至右得到每一位数字

//		
		if(temp==0 && t==len-1)				//如果最后一个数是0,则显示'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==0)	//如果前几位中出现0,且目前从未出现非零数,则显示' '
		{
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,' ',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==1)	//如果前几位中出现0,且曾经出现过非零数,则显示'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else								//(len-1)是保证全零时显示最末的0
		{
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
			else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		}
//		
		/*		
		if(enshow==0 && t<(len-1))			//(len-1)是保证全零时显示最末的0
		{
			if(temp==0)
			{
				if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);//0806宽为2+8/2=6
				else		oled_show_char(x + t*(lib/2),y,' ',lib,mode);//1206,1608宽都是高的1/2
				continue;//continue仅能跳过一次循环
			}
			else enshow=1;	 	 
		}
		else ;
		
	 	if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
		else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		*/
	}
}
//显示一组数字(高位的0依然显示)    /mode:0,反白显示  1,正常显示
void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u8 mode)
{         	
	u8 temp;						   
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//从左至右得到每一位数字
		if(lib==8)	oled_show_char(x + t*6,y,temp+'0',lib,mode);//0806宽为2+8/2=6
		else		oled_show_char(x + t*(lib/2),y,temp+'0',lib,mode);//1206,1608宽都是高的1/2
	}
}

/**
  * @brief  显示小数(可正可负)
  * @note	注意负号会占用第1格的空间, 默认不显示正号
  * @param  fnum: 想要显示的小数(float)
  *			len1: 其整数部分的长度, 注意负数会以为负号而右移显示1格
  *			len2: 其小数部分的长度, 即保留多少位小数
 *			mode: 0,反白显示  1,正常显示
  * @retval None
  */
void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u8 mode)
{
	u32 integer = 0;	//最终的小数里面的整数部分
	u16 pwr = 1;		//用于求出实际小数的次幂变量(1,10,100,1000, ...)
	u8 len22 = len2;	//因为len2在下方会自减, 所以要额外保存len2的初值
	while(len2)
	{
		pwr *= 10;
		len2--;
	}
	if(fnum < 0)//负数
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, '-', lib, mode);//在输入坐标左边1格显示负号
		
		fnum = fabs(fnum);
		
		integer = fnum;	//求出整数部分的值
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, mode);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, mode);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//求出小数部分的值
		oled_show_num_every0(x, y, integer, len22, lib, mode);
	}
	else		//正数
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, ' ', lib, mode);//在输入坐标左边1格显示空格方便覆盖掉负号
		
		integer = fnum;	//求出整数部分的值
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, mode);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, mode);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//求出小数部分的值
		oled_show_num_every0(x, y, integer, len22, lib, mode);
	}
}


/**
  * @brief  输入2Byte的GBK内码 -> 从SD卡读出点阵信息matrix -> 显示
  * @note	FatFs支持GBK编码, 但是长文件名必须是Unicode字符集的UTF-16格式
  *			所以FatFs里的的cc936.c将Unicode和GBK相互转化
  *			本函数仅需输入GBK码(2Byte), 其内部读取SD卡寻找点阵数据matrix[]
  *			【注】从上至下垂直扫描到底, 高位在前
  * @param  x, y: 坐标
  *			gbk : 16bit GBK 码值
  *			lib : 字体大小
 *			mode: 0,反白显示  1,正常显示
  * @retval None
  */
void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u8 mode)
{
	u8 temp, i, k;
	u8 y0 = y;
	u8 matrix[72] = {0};	//支持最大字号24 -> 24*24/8 = 72 Byte
	u8 csize = (lib/8 + ((lib%8)?1:0)) * lib;	//12->24Byte, 16->32Byte, 24->72Byte
	if((lib!=12)&&(lib!=16)&&(lib!=24))
		return;		//如果输入的字号不匹配, 则不显示
	else
	{
		//读取SD卡 得到对应字号 对应字码的 点阵数据
		ef_SD_GetGbkMatrix(gbk_H, gbk_L, lib, matrix, 0);// 0 = no fast seek
		
		for(i=0; i<csize; i++)
		{
			if((x>=128)||(y>=64))		continue;
			temp = matrix[i];
			for(k=0; k<8; k++)
			{
				if(temp & 0x80)		oled_draw_point(x, y, mode);
				else				oled_draw_point(x, y, !mode);
				temp <<= 1;
				y++;
				if((y - y0)==lib)
				{
					y = y0;
					x++;
					break;	//如果描完一列则跳至下一列的绘制
				}
			}
		}
	}
	
	
}


//显示字符串
//x,y:起点坐标    /lib:字体大小    /mode:0,反白显示  1,正常显示
void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u8 mode)
{	
	u8 i=0;
	u8 x0=x;
	while(p[i] != '\0')
	{		
		if(p[i]=='\r')							//如果遇到'\r'符
		{	
//			oled_show_char(x,y,' ',lib,mode);	//建议忽略 或者 显示一个' '
			i++;
			continue;							//跳过下方代码再进入while循环防止向右偏移造成间隔一个字符宽度
		}
		else if(p[i]=='\n')						//如果遇到了'\n'符
		{
//			oled_show_char(x,y,'\\',lib,mode);	//显示一个slash
			//oled_show_char(x,y,'n',lib,mode);	//显示一个'n'
			x = (lib==16)?0:2;					//换行显示 x+2 or x+0
			y = y+lib+1;						//换行显示 y+1
			i++;
			continue;							//跳过下方代码再进入while循环
		}
		else if(p[i]>=0x81)	//如果是中文
		{
			if(x>=128-lib/2+3)			//如果剩余的列数连一半汉字都不够显示则换行
			{
				x = x0;
				y = y + lib + 1;
			}
			else if(x>=128-lib+1)
			{
				//如果只剩半个汉字的空间, 只允许显示。，：；四个符号, 其余都应该即刻换行
				if(((p[i]!=0xA1)&&(p[i+1]!=0xA3))/* 。*/ || ((p[i]!=0xA3)&&(p[i]!=0xAC))/* ，*/ \
						|| ((p[i]!=0xA3)&&(p[i+1]!=0xBA))/* ：*/ || ((p[i]!=0xA3)&&(p[i+1]!=0xBB))/* ；*/)
				{
					x = x0;
					y = y + lib + 1;
				}
				else ;
			}
			else ;
			if((p[i]==0xFF)||(p[i+1]<0x40)||(p[i+1]==0x7F)||(p[i+1]==0xFF))//如果不在GBK内码范围内则返回
			{
				i += 2;//因为汉字是2Byte, 所以i应该多自加一次
				x += lib;
			}
			else
			{
				oled_show_GBK(x, y, p[i], p[i+1], lib, mode);
				i += 2;
				x += lib;
			}
			continue;
		}
		else
			oled_show_char(x, y, p[i], lib, mode);
		i++;
		if(lib==8) x += 6;
		else x += lib/2;
		
		/* 【已撤销此说法】若string超过右侧边界, 则换行继续显示 0806/1206:x+2, 1608:x+0, y+1 */
		/* 若string的右侧距离边界不足够显示下一个字符, 则换行显示 */
		/* 但是要注意对于 句号 和 逗号 其实可以模糊处理的, 毕竟他们不太合适换行显示 */
		if(((x>=123)&&(lib==8)&&(p[i]!='.')&&(p[i]!=',')) || ((x>=128-lib/2+1)&&(lib!=8)&&(p[i]!='.')&&(p[i]!=',')))
		{
			if(lib==16)
				x = x0;
			else
				x = x0 + 2;
			if(lib>=16)
				y = y + lib;
			else
				y = y + lib + 1;
		}
		else;
	}
}

/**
  * @brief  显示二进制图片(*.bin), 此函数目前没有普适性, 有时间把它改成适配可以显示任意横纵比的bin图片
  * @note	图片大小85*64, 从上至下垂直扫描到底, 高位在前, 注意字库全0为空格, 图片全0为白色图片
  *			也即图片全黑时winhex显示为全1 = 全部0XFF, 但对于oled黑色即不点亮像素(发送0)
  *			(64/8)*85 = 680Byte
  * @param  x, y: 	起点坐标
  *		   *pPic:	图片二进制信息, 或视频的一帧信息
  *			mode:  	/1,点亮像素点(推荐设置1)    /0,熄灭像素点
  * @retval None
  */
void oled_show_BinImage(u8 x, u8 y, const char *pPic, u8 mode)//3种badapple的bin格式: 085*64, 106*80, 170*128
{         	
	u8 temp;
	u8 y0 = y;
	for(u16 i=0; i<680; i++)	//可选 680, 1060, 2720
	{
		temp = pPic[i];
		for(u8 j=0;j<8;j++)
		{
			if(temp & 0x80)
				oled_draw_point(x, y, !mode);	//高位是1代表像素点原为黑色-> 熄灭像素点-> !mode=0
			else
				oled_draw_point(x, y, mode);
			temp <<= 1;
			y++;							//继续画下方的像素点				
			if((y - y0)== 64)	//可选 64, 80, 128
			{
				y = y0;
				x++;
				if(x>=128) return;			//超区域了(垂直扫描时横向越域必须return)
				else;
				break;
			}
			else;
			if(y>=64) break;				//超区域了(垂直扫描时竖向越域仅需break)
			else;
		}
	}
}

/**
  * @brief  显示二进制图片(*.bin)或数组图片(*.c), 此函数完全可以并入上面的Image函数
  * @note	此函数暂时拿来显示1:1的小图标, 暂定两个格式(48*48, 32*32)
  *			图片全黑时winhex为全1, 但应该向oled发送全0来关闭显示达到黑色的效果
  * @param  x, y: 	起点坐标
  *		    Icon:	图标号码(由oled.h的define定义), 指定图片二进制信息, 或视频的一帧信息
  *			mode:  	/1,点亮像素点(推荐设置1)    /0,熄灭像素点
  * @retval None
  */
void oled_show_Icon(u8 x, u8 y, u8 Icon, u8 height, u8 mode)
{
	const unsigned char *pPic;
	u8 temp;
	u8 y0 = y;
	switch(Icon){
		case 1:	pPic = icon_drone;	break;
		case 2: pPic = icon_sdcard; break;
		case 3: pPic = icon_mouse;	break;
		case 4: pPic = icon_remote; break;
		default: break;
	}
	for(u16 i=0; i< height*height/8; i++)
	{
		temp = pPic[i];
		for(u8 j=0;j<8;j++)
		{
			if(temp & 0x80)
				oled_draw_point(x,y,!mode);	//高位是1代表像素点原为黑色-> 熄灭像素点-> !mode=0
			else
				oled_draw_point(x,y,mode);
			temp <<= 1;
			y++;							//继续画下方的像素点				
			if((y - y0)== height)
			{
				y = y0;
				x++;
				if(x>=128) return;			//超区域了(垂直扫描时横向越域必须return)
				else;
				break;
			}
			else;
			if(y>=64) break;				//超区域了(垂直扫描时竖向越域仅需break)
			else;
		}
	}
}



//初始化SSD1306					    
void oled_ssd1306_init(void)//默认布局下 (Page_0,Column_0)在oled左上角的位置
{
	OLED_RST = 1;
	delay_ms(45);
	OLED_RST = 0;//低电平有效
	delay_ms(120);
	OLED_RST = 1;
	delay_ms(5);
	
	oled_write_Byte(0xAE,OLED_CMD);	//关闭显示
	oled_write_Byte(0x00,OLED_CMD);	//设置列的低地址
	oled_write_Byte(0x10,OLED_CMD);	//设置列的高地址
	oled_write_Byte(0x40,OLED_CMD); //设置显示开始行[5:0],行数.
	//oled_write_Byte(0xB0,OLED_CMD);//设置页首地址
	
	oled_write_Byte(0x81,OLED_CMD); //对比度设置
	oled_write_Byte(0x4F,OLED_CMD); //1~255; 默认0X7F(亮度设置,越大越亮)
	oled_write_Byte(0xA1,OLED_CMD); //0xA1默认布局,0xA0左右反置
	oled_write_Byte(0xC8,OLED_CMD); //0xC8默认布局,0xC0上下反置
	
	oled_write_Byte(0xA6,OLED_CMD); //0xA6全局正色显示,0xA7全局反色显示
	oled_write_Byte(0xA8,OLED_CMD); //设置驱动路数(1 to 64)
	oled_write_Byte(0X3F,OLED_CMD); //默认0x3F(1/64)
	oled_write_Byte(0xD3,OLED_CMD); //设置显示偏移
	oled_write_Byte(0X00,OLED_CMD); //默认偏移为0
	oled_write_Byte(0xD5,OLED_CMD); //设置时钟分频因子,震荡频率
	oled_write_Byte(0x80,OLED_CMD); //[3:0],分频因子; [7:4],震荡频率
	//oled_write_Byte(0xD8);//*** -set area color mode off
	//oled_write_Byte(0x05);//***
	oled_write_Byte(0xD9,OLED_CMD); //设置预充电周期
	oled_write_Byte(0xF1,OLED_CMD); //[3:0],PHASE 1 ; [7:4],PHASE 2
	oled_write_Byte(0xDA,OLED_CMD); //设置COM硬件引脚配置
	oled_write_Byte(0x12,OLED_CMD); //[5:4]配置
	
	
	oled_write_Byte(0xDB,OLED_CMD); //设置VCOM 电压倍率	
	oled_write_Byte(0x00,OLED_CMD); //0x00为0.65*VCC; 0x20为0.77*VCC; 0x30为0.83*VCC;(电压越高越亮)
	oled_write_Byte(0x20,OLED_CMD); //设置内存地址模式 
	oled_write_Byte(0x02,OLED_CMD); //0x00水平寻址,0x01垂直寻址,0x02页寻址(默认)
	oled_write_Byte(0x8D,OLED_CMD); //电荷泵设置
	oled_write_Byte(0x14,OLED_CMD); //泵的开启/关闭
	oled_write_Byte(0xA4,OLED_CMD); //0xA4为显示内存的内容,0xA5无视内存的内容
	oled_write_Byte(0xAF,OLED_CMD); //开启显示
	oled_clear();
	//oled_buffer_clear(0, 0, 127, 63);
}  



//Plexer先生已经验证过以上所有代码的正确性.
