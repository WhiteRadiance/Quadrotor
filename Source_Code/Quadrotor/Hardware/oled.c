#include "delay.h"
#include "oledfont.h"
#include "oled.h"
#include "spi.h"
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


//PA.234分别是Flash,SDcard,oled的CS端, PC.4是NRF的CS端.
//暂定OLED_CS(PC5),	OLED_RST(PB0), 注意OLED_DC(PC4)也同时连着led1, 而spi1的另三根线会在SPI1_Init()中得到初始化
void oled_spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
	/*
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_12;	//PA.234, PA3暂用于ADC1.CH3模拟输入模式
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//影响功耗
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_12);	//拉高所有CS端(PA口),关闭Flash,oled的spi
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;				//PC.4
 	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//复用推挽输出
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//影响功耗
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4);						//拉高NRF_CS端(PC4),关闭NRF24L01的spi
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				//PD.2 --> OLED_DC, led1
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD,GPIO_Pin_2);							//拉高, 即OLED_DC为1, led1熄灭	
	*/

	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;	//DC(PC4), CS(PC5)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//影响功耗
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);			//拉高
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//RES(PB0)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//推挽
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//影响功耗
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);						//拉高

	SPI1_Init();//初始化SPI1,(函数内配置了 Baud = 18M )
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
void buffer_clear(void)
{
	u8 y,x;		    
	for(y=0;y<8;y++)
		for(x=0;x<128;x++)
			Frame_Buffer[x][y] = 0x00;
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
		for(n=0;n<128;n++)
			oled_write_Byte(Frame_Buffer[n][i],OLED_DATA);
	}   
}

//清屏
void oled_clear(void)
{
	buffer_clear();
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
	temp = 0x01<<(offset);
	
	//因为仅对buffer操作, 并不需要locate()函数选定坐标!
	if(mode)	Frame_Buffer[x][page] |= temp;
	else			Frame_Buffer[x][page] &= ~temp;
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
		for(i=0;i<6;i++)
		{
			temp = ascii_0806[chr][i];
			for(k=0;k<8;k++)				//每次描8个点
			{
				if(temp&0x01) oled_draw_point(x,y,mode);//判断temp的末位是否为1
				else oled_draw_point(x,y,!mode);
				temp >>= 1; y++;			//准备操作下方的一个元素点
			}
			y = y0; x++;					//继续描右侧一列的点
		}
	}
	else if(lib==12 || lib==16)	//1206,1608
	{
		u8 csize=( lib/8+((lib%8)?1:0) ) * (lib/2);	//一个字符占的字节数,1206为12,1608为16
		for(i=0;i < csize;i++)
		{
			if(lib==12) temp = ascii_1206[chr][i];
			else				temp = ascii_1608[chr][i];
			if(i < csize/2)		//1206首次 描8点; 1608每次描8点
			{
				for(k=0;k<8;k++)
				{
					if(temp&0x01) oled_draw_point(x,y,mode);//判断temp的末位是否为1
					else 	oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//准备操作下方的一个元素点
				}
				y = y0; x++;						//继续描右侧一列的点
			}
			else				//1206第二次 描4点; 1608每次描8点
			{
				y = y0 + 8;
				x = x0 + i - csize/2;	//定位到开始第二次扫描的新坐标
				for(k=0;k<8;k++)
				{
					if(temp&0x01) oled_draw_point(x,y,mode);//判断temp的末位是否为1
					else 	oled_draw_point(x,y,!mode);
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
	else return;//无对应的字符集
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
			else	oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==0)	//如果前几位中出现0,且目前从未出现非零数,则显示' '
		{
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);
			else	oled_show_char(x + t*(lib/2),y,' ',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==1)	//如果前几位中出现0,且曾经出现过非零数,则显示'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else	oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else								//(len-1)是保证全零时显示最末的0
		{
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
			else				oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		}
//		
		/*		
		if(enshow==0 && t<(len-1))			//(len-1)是保证全零时显示最末的0
		{
			if(temp==0)
			{
				if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);//0806宽为2+8/2=6
				else	oled_show_char(x + t*(lib/2),y,' ',lib,mode);//1206,1608宽都是高的1/2
				continue;//continue仅能跳过一次循环
			}
			else enshow=1;	 	 
		}
		else ;
		
	 	if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
		else				oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
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
		else	oled_show_char(x + t*(lib/2),y,temp+'0',lib,mode);//1206,1608宽都是高的1/2
	}
}

//显示字符串(不能自动换行,请谨慎使用)
//x,y:起点坐标    /lib:字体大小    /mode:0,反白显示  1,正常显示
void oled_show_string(u8 x, u8 y, const char *p, u8 lib, u8 mode)
{	
	u8 i=0;
	while(p[i] != '\0')
	{		
		if(p[i]=='\r')							//如果遇到'\r'符
		{	
			oled_show_char(x,y,' ',lib,mode);	//建议忽略 或者 显示一个' '
			i++;
			continue;							//跳过下方代码再进入while循环防止向右偏移造成间隔一个字符宽度
		}
		else if(p[i]=='\n')						//如果遇到了'\n'符
		{
			oled_show_char(x,y,'\\',lib,mode);	//显示一个slash
			//oled_show_char(x,y,'n',lib,mode);	//显示一个'n'
			x=2; y=y+lib+1;						//换行显示+2,+1
			i++;
			continue;							//跳过下方代码再进入while循环
		}
		else
			oled_show_char(x,y,p[i],lib,mode);
		i++;
		if(lib==8) x += 6;
		else x += lib/2;
		
		/* 若string超过右侧边界, 则换行继续显示+2,+1 */
		if(x>=128){x=2; y=y+lib+1;}
		else;
	}
}

/**
  * @brief  显示二进制图片(*.bin), 此函数目前没有普适性, 有时间把它改成适配可以显示任意横纵比的bin图片
  * @note	图片大小85*64, 从上至下垂直扫描, 高位在前, 注意字库全0为空格, 图片全0为白色图片
  *			图片全黑时winhex显示为全部0XFF, 对于oled黑色即不点亮像素
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
	for(u16 i=0;i<680;i++)//可选 680, 1060, 2720
	{
		temp = pPic[i];
		for(u8 j=0;j<8;j++)
		{
			if(temp & 0x80)
				oled_draw_point(x,y,!mode);	//高位是1代表像素点原为黑色-> 熄灭像素点-> mode=0
			else
				oled_draw_point(x,y,mode);
			temp <<= 1;
			y++;							//继续画下方的像素点				
			if((y - y0)==64)//可选 64, 80, 128
			{
				y = y0;
				x++;
				if(x>=128) return;			//超区域了
				else;
				break;
			}
			else;
			if(y>=64) break;				//超区域了
			else;
		}
	}
}

//初始化SSD1306					    
void oled_ssd1306_init(void)//默认布局下 (Page_0,Column_0)在oled左上角的位置
{
	OLED_RST = 0;
	delay_ms(50);
	OLED_RST = 1;
	delay_ms(30);
	
	oled_write_Byte(0xAE,OLED_CMD);	//关闭显示
	oled_write_Byte(0x00,OLED_CMD);	//设置列的低地址
	oled_write_Byte(0x10,OLED_CMD);	//设置列的高地址
	oled_write_Byte(0x40,OLED_CMD); //设置显示开始行[5:0],行数.
	//oled_write_Byte(0xB0,OLED_CMD);//设置页首地址
	
	oled_write_Byte(0x81,OLED_CMD); //对比度设置
	oled_write_Byte(0x3F,OLED_CMD); //1~255;默认0X7F(亮度设置,越大越亮)
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
	
	
	oled_write_Byte(0xDB,OLED_CMD); //设置VCOMH 电压倍率
	oled_write_Byte(0x00,OLED_CMD); //0x00为0.65*VCC; 0x20为0.77*VCC; 0x30为0.83*VCC;(电压越高越亮)
	oled_write_Byte(0x20,OLED_CMD); //设置内存地址模式 
	oled_write_Byte(0x02,OLED_CMD); //0x00水平寻址,0x01垂直寻址,0x02页寻址(默认)
	oled_write_Byte(0x8D,OLED_CMD); //电荷泵设置
	oled_write_Byte(0x14,OLED_CMD); //泵的开启/关闭
	oled_write_Byte(0xA4,OLED_CMD); //0xA4为显示内存的内容,0xA5无视内存的内容
	oled_write_Byte(0xAF,OLED_CMD); //开启显示
	delay_ms(10);
	buffer_clear();
}  



//Plexer先生已经验证过以上所有代码的正确性.
