#ifndef _OLED_H
#define _OLED_H
#include "sys.h"

/* Choose one type of OLED_Drive_IC */
//#define SSD1306_C128_R64
#define SSD1351_C128_R128
//#define SSD1351_C128_R96


//48*48 pixel, 数组数据是原图片的颜色取反之后得到的
extern const u8 icon_drone[288];
extern const u8 icon_sdcard[288];
extern const u8 icon_mouse[288];
extern const u8 icon_remote[288];
//#define Icon_drone		1
//#define Icon_sdcard		2
//#define Icon_mouse		3
//#define Icon_remote		4


#if defined		SSD1306_C128_R64
	#define WIDTH	128
	#define HEIGHT	64
#elif defined	SSD1351_C128_R96
	#define WIDTH	128
	#define HEIGHT	96
#elif defined	SSD1351_C128_R128
	#define WIDTH	128
	#define HEIGHT	128
#else
	#error Your oled driver IC may be excluded in oled header, please check.
#endif

#if !defined SSD1306_C128_R64
	/* RGB888 -> RGB565 转换宏 */
//	#define RGB(R,G,B) (((R>>3)<<11) | ((G>>2)<<5) | (B>>3))
//	enum
//	{
//		BLACK	= RGB(255,255,255),
//		WHITE	= RGB(  0,  0,  0),
//		RED		= RGB(255,  0,  0),
//		GREEN	= RGB(  0,255,  0),
//		BLUE	= RGB(  0,  0,255),
//		CYAN	= RGB(  0,255,255),
//		LPURPLE	= RGB(255,  0,255),
//		YELLOW	= RGB(255,255,  0)
//	};

	/* RGB565 format */
/*	enum
	{
		BLACK	= 0x0000,
		WHITE	= 0xFFFF,
		RED		= 0xF800,
		GREEN	= 0x07E0,
		BLUE	= 0x001F,	//颜色很深的蓝色
		CYAN	= 0x07FF,	//青色(maxG+maxB)->接近单色OLED的颜色
		LPURPLE	= 0xF81F,	//亮紫色(maxR+maxB)->比粉色深,比紫色淡(light purple)
		YELLOW	= 0xFFE0,	//黄色(maxR+maxG)
		
		PURPLE	= 0x780F,	//紫色
		DGRAY	= 0x7BEF,	//深灰色
		LGRAY	= 0xC618	//白灰色
	};*/
#endif

#define	OLED_CMD	0		//写命令
#define OLED_DATA	1		//写数据

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
#define OLED_CS   	PBout(0)	//对应CS, spi_CS
#define OLED_DC 	PCout(5)	//对应DC, Data/Command
#define OLED_RST 	PCout(4)	//对应RST,oled的硬件复位, rst=0是复位


//OLED宏观控制函数
void oled_spi_init(void);
void oled_write_Byte(u8 dat,u8 cmd);
void oled_display_on(void);
void oled_display_off(void);
void oled_refresh(void);		//将buffer中的一帧数据送往GRAM显示
void oled_clear(void);			//buffer_clear() + oled_refresh()


#if defined 	SSD1306_C128_R64
	void oled_ssd1306_init(void);
//以下函数必须配合refresh()函数使用!
	void oled_draw_point(u8 x, u8 y, u16 mode);//x<128,y<63   /mode:1 点亮像素点  0: 熄灭像素点
	//以下函数中 mode=0,反白显示  1,正常显示
	void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u16 mode);			//在(x,y)显示字符
	void oled_show_num_hide0( u8 x, u8 y, u32 num, u8 len, u8 lib, u16 mode);//高位的0不显示
	void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 mode);//所有0都被显示
	//显示小数(仅显示负号)    /len1:整数部分长度  len2:小数部分长度
	void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u16 mode);
	void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u16 mode);
	void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u16 mode);//根据GBK内码显示一个汉字
	void oled_show_Bin_Array(u8 x, u8 y, const u8 *pPic, u16 mode);
	
#elif defined 	SSD1351_C128_R96 || defined SSD1351_C128_R128
	void oled_ssd1351_init(void);
//以下函数必须配合refresh()函数使用!
	void oled_draw_point(u8 x, u8 y, u16 color);
	void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u16 front_color, u16 back_color);
	void oled_show_num_hide0( u8 x, u8 y, u32 num, u8 len, u8 lib, u16 front_color, u16 back_color);
	void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 front_color, u16 back_color);
	//显示小数(仅显示负号)    /len1:整数部分长度  len2:小数部分长度
	void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u16 front_color, u16 back_color);
	void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u16 front_color, u16 back_color);
	void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u16 front_color, u16 back_color);
	void oled_show_Bin_Array(u8 x, u8 y, const u8 *pPic, u8 width, u8 height, u8 depth, u8 mono_byte_turn);
#endif

void oled_buffer_clear(u8 x0, u8 y0, u8 x1, u8 y1);	//将Frame缓存数组中的确定区域 置零
u32	 mypow(u8 m,u8 n);	//m^n次幂


//void oled_show_Icon(u8 x, u8 y, const u8 Icon, u8 height, u16 mode);

//void oled_show_ColorImage(u8 x, u8 y, const u8 *pPic, u8 width, u8 height);



//OLED数学绘图函数(使用彩色屏时exe实际是color)
//以下函数调用了draw_point(),显示时记得调用refresh()
void oled_draw_line(u8 x0,u8 y0,u8 x1,u8 x2,u16 exe);		//两点连线    	/exe=1执行  0删除
void oled_draw_circle(u8 x0,u8 y0,u8 r,u16 exe);			//Bresenham算法	/exe=1执行  0删除
void oled_draw_rectangle(u8 x1,u8 y1,u8 x2,u8 y2,u16 exe);	//画矩形		/exe=1执行  0删除





#define		BLACK		0x0000		//BLACK(OFF)
#define		Navy		0x000F		//DARK BLUE
#define		Dgreen		0x03E0		//DARK GREEN
#define		Dcyan		0x03EF		//DARK CYAN
#define		Maroon		0x7800		//DARK RED
#define		Purple		0x780F		//PURPLE
#define		Olive		0x7BE0		//橄榄GREEN
#define		Lgray		0xC618		//WHITE GRAY
#define		Dgray		0x7BEF		//DARK GRAY
#define		Blue		0x001F		//BLUE
#define		Green		0x07E0		//GREEN
#define		CYAN		0x07FF		//CYAN
#define		Red			0xF800		//RED
#define		Magenta		0xF81F		//LIGHT PURPLE (Fake Magenta)
#define		Yellow		0xFFE0		//YELLOW
#define		White		0xFFFF		//WHITE


#endif 
