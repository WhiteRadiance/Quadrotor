#ifndef _PICTURELIB_H
#define _PICTURELIB_H

#include "fmt_bmp.h"
#include "fmt_jpeg.h"


//RGB565和RGB888的互相转换最好使用线性压扩(如下),而不推荐直接截断或移位扩展(有误差)  转换函数移至fmt_jpeg.c
//参考博客: https://blog.csdn.net/happy08god/article/details/10516871 

//RGB565 -> RGB888
//R8 = ( R5 * 527 + 23 ) >> 6;
//G8 = ( G6 * 259 + 33 ) >> 6;
//B8 = ( B5 * 527 + 23 ) >> 6;

//RGB888 -> RGB565
//R5 = ( R8 * 249 + 1014 ) >> 11;
//G6 = ( G8 * 253 +  505 ) >> 10;
//B5 = ( B8 * 249 + 1014 ) >> 11;



void ef_multimedia_pic_bmp(char* path_t);		//bmp

void ef_multimedia_pic_jpeg(char* path_t);		//jpeg



#endif

