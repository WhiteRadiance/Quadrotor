#ifndef __OLED_H
#define __OLED_H			  	 
#include "sys.h"
		   
#define	OLED_CMD   0		//写命令
#define OLED_DATA  1		//写数据
//4线spi接口时使用
/*
#define OLED_SCLK 	PAout(5)	//对应D0, spi的串行时钟
#define OLED_SDIN 	PAout(7)	//对应D1, spi模式下只能写(MOSI)
#define OLED_CS   	PAout(4)	//对应CS, spi片选
#define OLED_DC 	PDout(2)	//对应DC, 命令/数据, 注意PD2还同时连接着 led1 !!!
#define OLED_RST 	PAout(12)	//对应RST,oled的硬件复位, rst=0是复位
*/
#define OLED_SCLK 	PAout(5)	//对应D0, spi_SCK
#define OLED_SDIN 	PAout(7)	//对应D1, spi_MOSI
#define OLED_CS   	PCout(5)	//对应CS, spi_CS
#define OLED_DC 	PCout(4)	//对应DC, Data/Command
#define OLED_RST 	PBout(0)	//对应RST,oled的硬件复位, rst=0是复位

//OLED控制用函数
void oled_spi_init(void);
void oled_ssd1306_init(void);
void oled_write_Byte(u8 dat,u8 cmd);
void oled_display_on(void);
void oled_display_off(void);
void buffer_clear(void);		//将帧缓存数组置零
void oled_refresh(void);		//将buffer中的一帧数据送往GRAM显示
void oled_clear(void);

//以下函数必须配合refresh()函数使用!
void oled_draw_point(u8 x, u8 y, u8 mode);							//x<128,y<63   /mode:1 点亮像素点  0: 熄灭像素点
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u8 mode);			//任意(x,y)显示字符,   /mode=0,反白显示  1,正常显示
//u32 mypow(u8 m,u8 n);//m^n次幂
void oled_show_num_hide0( u8 x,u8 y,u32 num,u8 len,u8 lib,u8 mode);	//高位的0不显示    /mode=0,反白显示  1,正常显示
void oled_show_num_every0(u8 x,u8 y,u32 num,u8 len,u8 lib,u8 mode);	//所有0都被显示    /mode=0,反白显示  1,正常显示
void oled_show_string(u8 x, u8 y, const char *p, u8 lib, u8 mode);	//mode=0,反白显示  1,正常显示
void oled_show_BinImage(u8 x, u8 y, const char *pPic, u8 mode);

//OLED数学绘图函数
//以下函数调用了draw_point(),显示时记得调用refresh()
void oled_draw_line(u8 x0,u8 y0,u8 x1,u8 x2,u8 exe);		//两点连线    	/exe=1执行画线  0删除线条
void oled_draw_circle(u8 x0,u8 y0,u8 r,u8 exe);				//Bresenham算法画圆	  /exe=1执行  0删除
void oled_draw_rectangle(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe);	//画矩形		/exe=1执行  0删除

#endif 
