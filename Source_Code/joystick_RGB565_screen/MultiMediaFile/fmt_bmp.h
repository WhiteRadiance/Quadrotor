#ifndef _FMT_BMP_H
#define _FMT_BMP_H

/* ==================================================================================================== */
/*											BMP 图片格式 结构体											*/
/*			Windows扫描BMP的最下单位是4Byte，因此要求每行数据要4字节对齐，不足4字节整数倍要补0			*/
/*			 计算公式为： 每行补0后实际字节数 = ((图片宽度像素数 * 像素对应bit数 + 31)/32)*4			*/
/*	例如：色深1b每行85像素时每行85bit=补0后每行12字节；色深16b每行85像素时每行1360bit=补0后每行172字节	*/
/* ---------------------------------------------------------------------------------------------------- */


typedef unsigned char 		u8;
typedef unsigned short int	u16;
typedef unsigned int 		u32;


struct s_BMP_header{			//14 Byte --- BMP文件头
	u16 Type;					//文件类型(可取BM,BA,CI,CP,IC,PT)
	u32 Size;					//文件大小(Byte)
	u16 Reserved1;				//保留,必须为0
	u16 Reserved2;				//保留,必须为0
	u32 Offbits;				//位图数据偏移地址(即使有调色板也会被偏移过去)
};

struct s_BMP_info_header{		//40 Byte --- BMP信息头(不过16b-565的信息头大小都是56,也就是把16Byte调色板也算进去了)
	u32 biSize;					//info_header结构体的大小(常用的只有40Byte,旧版包括12,64,108,124)
	u32 biWidth;				//图片宽度(pixel)
	u32 biHeight;				//图片高度(正数:底到顶; 负数:顶到底)(一般都是正数)
	u16 biPlanes;				//颜色平面数(总是1)
	u16 biBitCount;				//色深(1,4,8,16,24,32)
	u32 biCompress;				//图片压缩类型(0:RGB(不压缩)(常用); 1:RLE8(8位BMP); 2:RLE4(4位BMP);
								//3:BitFields(比特域,16/32位图); 4/5:BMP包含JPG/PNG(仅用于打印机))
	u32 biSizeImage;			//4字节对齐的图片大小
	u32 biXPelsPerMeter;		//水平分辨率(pixel/m)
	u32 biYPelsPerMeter;		//垂直分辨率(pixel/m)
	u32 biClrUsed;				//BMP实际使用的调色板索引数目(如果设为0,代表使用所有调色板索引)
	u32 biClrImportant;			//对图片显示很重要的调色板索引数目(如果设为0,代表所有索引都重要)
};

struct s_BMP_RGB_quad{			//24/32位BMP没有调色板,16位色图一般也没有调色板
	u8  rgbBlue;				//biBitCount指示quad结构体的个数(如=1代表有2个quad,如=8代表有256个quad)
	u8  rgbGreen;				//16位BMP的biCompression=0时代表没有调色板(quad),=3时quad根据555和565变成4个32bit的掩码
	u8  rgbRed;					//555-RGB是16位BMP默认格式(MSbit=0); 565-RGB掩码:0x00F800/0x0007E0/0x00001F(注意小端模式)
	u8  Alpha;					//Alpha若不设置透明通道时设置为0
};

struct s_BMP_565_MASK{			//不属于标准的BMP文件头
	u8  mask[16];				//用于保存RGB565的掩码:0x00F800/0x0007E0/0x00001F/Alpha:0x0000(注意小端模式)
};



#endif
