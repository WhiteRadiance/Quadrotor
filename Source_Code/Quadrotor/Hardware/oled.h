#ifndef __OLED_H
#define __OLED_H			  	 
#include "sys.h"
		   
#define	OLED_CMD   0		//д����
#define OLED_DATA  1		//д����
//4��spi�ӿ�ʱʹ��
/*
#define OLED_SCLK 	PAout(5)	//��ӦD0, spi�Ĵ���ʱ��
#define OLED_SDIN 	PAout(7)	//��ӦD1, spiģʽ��ֻ��д(MOSI)
#define OLED_CS   	PAout(4)	//��ӦCS, spiƬѡ
#define OLED_DC 	PDout(2)	//��ӦDC, ����/����, ע��PD2��ͬʱ������ led1 !!!
#define OLED_RST 	PAout(12)	//��ӦRST,oled��Ӳ����λ, rst=0�Ǹ�λ
*/
#define OLED_SCLK 	PAout(5)	//��ӦD0, spi_SCK
#define OLED_SDIN 	PAout(7)	//��ӦD1, spi_MOSI
#define OLED_CS   	PCout(5)	//��ӦCS, spi_CS
#define OLED_DC 	PCout(4)	//��ӦDC, Data/Command
#define OLED_RST 	PBout(0)	//��ӦRST,oled��Ӳ����λ, rst=0�Ǹ�λ

//OLED�����ú���
void oled_spi_init(void);
void oled_ssd1306_init(void);
void oled_write_Byte(u8 dat,u8 cmd);
void oled_display_on(void);
void oled_display_off(void);
void buffer_clear(void);		//��֡������������
void oled_refresh(void);		//��buffer�е�һ֡��������GRAM��ʾ
void oled_clear(void);

//���º����������refresh()����ʹ��!
void oled_draw_point(u8 x, u8 y, u8 mode);							//x<128,y<63   /mode:1 �������ص�  0: Ϩ�����ص�
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u8 mode);			//����(x,y)��ʾ�ַ�,   /mode=0,������ʾ  1,������ʾ
//u32 mypow(u8 m,u8 n);//m^n����
void oled_show_num_hide0( u8 x,u8 y,u32 num,u8 len,u8 lib,u8 mode);	//��λ��0����ʾ    /mode=0,������ʾ  1,������ʾ
void oled_show_num_every0(u8 x,u8 y,u32 num,u8 len,u8 lib,u8 mode);	//����0������ʾ    /mode=0,������ʾ  1,������ʾ
void oled_show_string(u8 x, u8 y, const char *p, u8 lib, u8 mode);	//mode=0,������ʾ  1,������ʾ
void oled_show_BinImage(u8 x, u8 y, const char *pPic, u8 mode);

//OLED��ѧ��ͼ����
//���º���������draw_point(),��ʾʱ�ǵõ���refresh()
void oled_draw_line(u8 x0,u8 y0,u8 x1,u8 x2,u8 exe);		//��������    	/exe=1ִ�л���  0ɾ������
void oled_draw_circle(u8 x0,u8 y0,u8 r,u8 exe);				//Bresenham�㷨��Բ	  /exe=1ִ��  0ɾ��
void oled_draw_rectangle(u8 x1,u8 y1,u8 x2,u8 y2,u8 exe);	//������		/exe=1ִ��  0ɾ��

#endif 
