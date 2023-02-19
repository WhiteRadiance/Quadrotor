//#include "extfunc.h"
#include "PictureLib.h"
#include "ff.h"
//#include "sdio_sdcard.h"
#include "oled.h"
#include "delay.h"
//#include <string.h>
//#include <stdio.h>
#include <stdlib.h>			//malloc
/*【说明】：
/  该c文件包含 BMP,PNG,JPG 图片格式的显示，用于扩展extfunc.c中的多媒体图片查看器	*
/  核心的函数都分别针对不同的图片格式，函数命名格式：ef_multimedia_pic_<FORMAT>		*/

/**
  *************************************************************************************************
  *											定义变量
  *************************************************************************************************
  */
extern FRESULT	f_res;			//File function return code (FRESULT), 定义于extfunc.c
extern FATFS 	fs;				//FatFs文件系统结构体, 定义于extfunc.c
extern FIL		fp;				//文件结构体指针, 定义于extfunc.c
extern UINT		bw,br;			//当前总共写入/读取的字节数(用于f_read(),f_write())
extern FILINFO	fno;			//文件信息, 定义于extfunc.c


#if defined SSD1306_C128_R64
	extern u8	Frame_Buffer[128][8];
#elif defined SSD1351_C128_R128
	extern u16	Frame_Buffer[128][128];
#elif defined SSD1351_C128_R96
	extern u16	Frame_Buffer[96][128];
#endif


/**
  *************************************************************************************************
  *											定义函数
  *************************************************************************************************
  */

/**************************************************************************************************
  * @brief  用bmp的数组buf去更新Frame[]数组
  * @note	总体上是行扫描,但是列是逆序的,即从底部至顶部
			仅用于单色bmp,仅用于此c文件内部使用
  * @param  类似于oled_show_Bin_Array()
  * @retval None
  ************************************************************************************************/
/*
static void PicLib_show_bmp_buf(u8 x, u8 y, const u8 *pPic, u8 width, u8 height, u8 depth)
{
	if(depth != 0x01)
		return;
	u16 n = 0;
	u8  temp;
	for(u8 j=1; j<=height; j++){								//逆序列的行扫描(典型的BMP方式)
		for(u8 i=0; i<width; i++){
			temp = 0x80;
			for(u8 t=0; t<8; t++){
				if(pPic[n] & temp){								//读取bin得到的0xFF代表黑色
					Frame_Buffer[height-j+y][i*8+x+t] = BLACK;
					//Frame_Buffer[j+y][(i*8+x+t)*2+1] = (u8)BLACK;
				}
				else{
					Frame_Buffer[height-j+y][i*8+x+t] = CYAN;
					//Frame_Buffer[j+y][(i*8+t+x)*2+1] = (u8)CYAN;
				}
				if(i*8+x+t+1 >= WIDTH)
					break;
				if(i*8+t+1 == width)							//如果显示到图片宽度了, 补0的数据跳过显示
					break;
				
				temp >>= 1;
			}
			if(j+y+1 >= HEIGHT)
				return;
			n += 1;
		}
	}
}
*/
/**************************************************************************************************
  * @brief  图片查看器下辖的bmp图片查看器
  * @note	函数先读取文件格式信息，然后显示BMP中的实际数据
			BMP文件头的格式主要包含：(详细的文件头见 PictureLib.h 的结构体)
				BMP_Header:		文件头(14Byte)	BM标签,文件大小,偏移地址...
				BMP_InfoHeader: 信息头(40Byte)	图片尺寸,色深,色彩bit域,调色板索引号...
				BMP_RGB_quad:	调色板(可选)	索引号对应某个调色板的颜色值
				BMP_ColorData:	图片数据		由颜色值(或调色板索引)组成的信息
  * @param  path: bin文件完整路径
  * @retval None
  ************************************************************************************************/
void ef_multimedia_pic_bmp(char* path_t)
{
	u8  bmp_head[54]={0};				//保存54Byte文件头(info_head不是40Byte则判断出错)
//	u8 	pic_width=0, pic_height=0;		//图片的长宽
//	u8 	pic_color_depth=0;				//图片色彩深度(目前只判断单色和16bit深度)

	u8	origin_x=0, origin_y=0;			//根据长宽自动计算左上角的坐标(居中显示)
	u8	pic_buf[2048];					//128*128的单色图片只消耗128*128/8=2048Byte(而彩色图片不能完全装得下)
	
	oled_buffer_clear(0, 0, WIDTH-1, HEIGHT-1);
	f_res = f_open(&fp, path_t, FA_READ | FA_OPEN_EXISTING);
	if(f_res==FR_OK)
	{
		struct s_BMP_header			Header;
		struct s_BMP_info_header	InfoHdr;
//		struct s_BMP_RGB_quad		RGBquaSd;	//refer to line-106
		
		f_res = f_read(&fp, bmp_head, 54, &br);
		
		Header.Type			= bmp_head[ 1] + ((u16)bmp_head[ 0]<<8);
		Header.Size			= bmp_head[ 2] + ((u16)bmp_head[ 3]<<8) + ((u32)bmp_head[ 4]<<16) + ((u32)bmp_head[ 5]<<24);
		Header.Offbits		= bmp_head[10] + ((u16)bmp_head[11]<<8) + ((u32)bmp_head[12]<<16) + ((u32)bmp_head[13]<<24);
		
		InfoHdr.biSize		= bmp_head[14] + ((u16)bmp_head[15]<<8) + ((u32)bmp_head[16]<<16) + ((u32)bmp_head[17]<<24);
		InfoHdr.biWidth		= bmp_head[18] + ((u16)bmp_head[19]<<8) + ((u32)bmp_head[20]<<16) + ((u32)bmp_head[21]<<24);
		InfoHdr.biHeight	= bmp_head[22] + ((u16)bmp_head[23]<<8) + ((u32)bmp_head[24]<<16) + ((u32)bmp_head[25]<<24);
		InfoHdr.biPlanes	= bmp_head[26] + ((u16)bmp_head[27]<<8);
		InfoHdr.biBitCount	= bmp_head[28] + ((u16)bmp_head[29]<<8);
		InfoHdr.biCompress	= bmp_head[30] + ((u16)bmp_head[31]<<8) + ((u32)bmp_head[32]<<16) + ((u32)bmp_head[33]<<24);
		InfoHdr.biSizeImage	= bmp_head[34] + ((u16)bmp_head[35]<<8) + ((u32)bmp_head[36]<<16) + ((u32)bmp_head[37]<<24);

		if(Header.Type != 0x424D){																	//没有发现"BM"标签
			oled_show_string( 0, 0, (u8*)"No 'BM' tag", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
//		if(InfoHdr.biSize != 0x00000028){															//BMP_INFO_HEADER的大小必须是40Byte
//			oled_show_string( 0, 0, (u8*)"InfoHead != 40", 12, BLACK, CYAN);						//不过16b-565的大小有时是56Byte
//			f_close(&fp);																			//为了兼容性考量,此段先注释掉
//			return;
//		}
		if((InfoHdr.biWidth>WIDTH) || (InfoHdr.biHeight>HEIGHT)){									//还不支持下采样，图片尺寸太大会报错
			oled_show_string( 0, 0, (u8*)"picture too large", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
		if((InfoHdr.biBitCount!=0x0001) && (InfoHdr.biBitCount!=16) && (InfoHdr.biBitCount!=24)){	//只支持1b,16b,24b色彩深度
			oled_show_string( 0, 0, (u8*)"unsupported depth", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
		origin_x = (WIDTH  - InfoHdr.biWidth)/2;													//确定居中显示的左上角坐标点
		origin_y = (HEIGHT - InfoHdr.biHeight)/2;
		
		u16 Raw_TotalByte_with0 = 0;																//bmp补0后每行占的字节数
		u16 Raw_Extra_zerosByte	= 0;																//bmp每行被补了多少字节0
		
		if(InfoHdr.biBitCount == 0x0001)			// =================================如果bmp的色深为 1bit ==================================
		{
			u8* qd = (u8*)malloc(sizeof(u8) * 8);													//malloc
			struct s_BMP_RGB_quad quad[2];															//存储调色板
			f_read(&fp, qd, 8, &br);
			quad[0].rgbRed	= qd[0];		quad[1].rgbRed	= qd[4];
			quad[0].rgbGreen= qd[1];		quad[1].rgbGreen= qd[5];
			quad[0].rgbBlue	= qd[2];		quad[1].rgbBlue	= qd[6];
			quad[0].Alpha	= qd[3];		quad[1].Alpha	= qd[7];
			free(qd);																				//free
			qd = NULL;			
			
			Raw_TotalByte_with0 = ((InfoHdr.biWidth * InfoHdr.biBitCount + 31) >> 5) << 2;			//计算出该bmp图像每行占的字节数
			Raw_Extra_zerosByte = Raw_TotalByte_with0 - ((InfoHdr.biWidth+7) >> 3);					//计算出暂存时应滤掉多少字节的0
				
			for(u8 i=0; i<InfoHdr.biHeight; i++){													//行扫描读取, (用指针)滤掉被补的0
				f_res = f_read(&fp, pic_buf+i*(Raw_TotalByte_with0-Raw_Extra_zerosByte), Raw_TotalByte_with0, &br);
				br += br;
			}
			if(f_res==FR_OK){
				//1bit色深的调色板只有两个颜色,即对应0和1号索引(比如后面数据里的1或0xFF都指代1号索引)
				//简单起见,判断到0号索引的quad_Red=0xFF则认为0号索引代表白色(虽然读到RGB都是0xFF才是白色)
				//那么,下一个颜色1号索引就自然对应RGB都是0x00,即黑色
				if(quad[0].rgbRed == 0x00){
					for(u16 t=0; t<InfoHdr.biHeight*(Raw_TotalByte_with0-Raw_Extra_zerosByte); t++)	//保证pic_buf里的1代表黑色像素
						pic_buf[t] = ~pic_buf[t];
				}
				else ;
				
				//PicLib_show_bmp_buf(origin_x, origin_y, pic_buf, InfoHdr.biWidth, InfoHdr.biHeight, InfoHdr.biBitCount);
				
				
				
				
				
				
				
				
				u16 n = 0;
				u8  temp;
				for(u8 j=1; j<=InfoHdr.biHeight; j++){								//逆序列的行扫描(典型的BMP方式)
					for(u8 i=0; i<(InfoHdr.biWidth+7)>>3; i++){
						temp = 0x80;
						for(u8 t=0; t<8; t++){
							
							if(i*8+origin_x+t >= WIDTH)		break;
							if(j+origin_y >= HEIGHT)		break;
							
							if(pic_buf[n] & temp){									//读取bin得到的0xFF代表黑色
								Frame_Buffer[InfoHdr.biHeight-j+origin_y][i*8+origin_x+t] = BLACK;
								//Frame_Buffer[j+y][(i*8+x+t)*2+1] = (u8)BLACK;
							}
							else{
								Frame_Buffer[InfoHdr.biHeight-j+origin_y][i*8+origin_x+t] = CYAN;
								//Frame_Buffer[j+y][(i*8+t+x)*2+1] = (u8)CYAN;
							}

							if(i*8+t+1 == InfoHdr.biWidth)							//如果显示到图片宽度了, 补0的数据跳过显示
								break;
							
							temp >>= 1;
						}
						n += 1;
					}
				}
				
				
				
				
				
				
				
				
				oled_refresh();
			}
			else if(br < InfoHdr.biSizeImage)
				oled_show_string(0, 13, (u8*)"error: br < btr.", 12, BLACK, CYAN);
			else
				oled_show_string(0, 26, (u8*)"file can't read.", 12, BLACK, CYAN);

		}
		else if(InfoHdr.biBitCount == 16)			// ================================= 如果bmp的色深为 16bit =================================
		{
			u16 n = 0;
			
			Raw_TotalByte_with0 = ((InfoHdr.biWidth * InfoHdr.biBitCount + 31) >> 5) << 2;			//计算出该bmp图像每行占的字节数
//			Raw_Extra_zerosByte = Raw_TotalByte_with0 - InfoHdr.biWidth*2;							//计算出暂存时应滤掉多少字节的0
			
			if(InfoHdr.biCompress == 0)				// ================================= RGB-555(Default) =====
			{
				for(u8 i=1; i<=InfoHdr.biHeight; i++){												//行扫描读取,直接存到Frame[]中(所以忽略补0)
					f_res = f_read(&fp, pic_buf, Raw_TotalByte_with0, &br);
					n = 0;
					for(u8 j=0; j<InfoHdr.biWidth; j++){
						
						if(j+origin_x >= WIDTH)		break;											//从底部开始显示,注意溢出
						if(i+origin_y >= HEIGHT)	break;
						
						//RGB555 -> RGB565 (G的末位是1则追加1, G的末位是0则只移位)
						u8  val_RG = pic_buf[n+1];													//16bit中包含R,G色的那个字节
						u8  val_GB = pic_buf[n];													//16bit中包含G,B色的那个字节
						u8  temp = val_GB & 0x1F;													//暂时保存5bit的 Blue 成分
						val_RG <<= 1;																//转换成显示器支持的RGB-565格式的颜色
						val_RG |= val_GB>>7;
						if(val_GB & 0x20)	val_GB = ((val_GB&0xE0)<<1) + 0x20 + temp;
						else				val_GB = ((val_GB&0xE0)<<1) + temp;
																									//注意RGB格式(bmp order: B-G-R)
						Frame_Buffer[InfoHdr.biHeight-i+origin_y][j+origin_x] = val_GB + ((u16)val_RG<<8);
						
						n += 2;
					}
				}
			}
			else if(InfoHdr.biCompress == 0x0003)	// ================================= RGB-565(With Mask) =====
			{
				struct s_BMP_565_MASK RGB565;
//				oled_show_num_every0( 0, 13, InfoHdr.biWidth, 3, 12, BLACK, CYAN);
//				oled_show_num_every0( 0, 26, InfoHdr.biHeight, 3, 12, BLACK, CYAN);
//				oled_refresh();
//				while(1);
				f_read(&fp, RGB565.mask, 16, &br);													//读取R,G,B,Alpha的掩码
				u32 MASK_R = RGB565.mask[ 0] + ((u16)RGB565.mask[ 1]<<8) + ((u32)RGB565.mask[ 2]<<16) + ((u32)RGB565.mask[ 3]<<24);
				u32 MASK_G = RGB565.mask[ 4] + ((u16)RGB565.mask[ 5]<<8) + ((u32)RGB565.mask[ 6]<<16) + ((u32)RGB565.mask[ 7]<<24);
				u32 MASK_B = RGB565.mask[ 8] + ((u16)RGB565.mask[ 9]<<8) + ((u32)RGB565.mask[10]<<16) + ((u32)RGB565.mask[11]<<24);
				u32 MASK_A = RGB565.mask[12] + ((u16)RGB565.mask[13]<<8) + ((u32)RGB565.mask[14]<<16) + ((u32)RGB565.mask[15]<<24);
				if((MASK_R!=0x0000F800) && (MASK_G!=0x000007E0) && (MASK_B!=0x0000001F) && (MASK_A!=0x00000000)){
					oled_show_string( 0, 26, (u8*)"Unknown 565 Mask", 12, BLACK, CYAN);
					f_close(&fp);
					return;
				}
				
				for(u8 i=1; i<=InfoHdr.biHeight; i++){												//行扫描读取, (用指针)滤掉被补的0
					f_res = f_read(&fp, pic_buf, Raw_TotalByte_with0, &br);
					n = 0;
					for(u8 j=0; j<InfoHdr.biWidth; j++){
						
						if(j+origin_x >= WIDTH)		break;											//从底部开始显示,注意溢出
						if(i+origin_y >= HEIGHT)	break;
																									//注意逆序读取(bmp order: B-G-R)
						Frame_Buffer[InfoHdr.biHeight-i+origin_y][j+origin_x] = pic_buf[n] + ((u16)pic_buf[n+1]<<8);

						n += 2;
					}
				}
			}
			else
			{
				oled_show_string( 0, 26, (u8*)"Special 16b bmp", 12, BLACK, CYAN);
				f_close(&fp);
				return;
			}
			oled_refresh();

		}
		else if(InfoHdr.biBitCount == 24)		// ================================= 如果bmp的色深为 24bit =================================
		{
			oled_show_string( 0, 39, (u8*)"It is a 24b bmp", 12, BLACK, CYAN);
		}
		else ;

		f_close(&fp);
	}
	else
		oled_show_string(0, 0, (u8*)"File path error.", 12, BLACK, CYAN);
}







/**************************************************************************************************
  * @brief  图片查看器下辖的jpeg图片查看器
  * @note	函数凭借fmt_jpeg.c对文件头进行解析,然后解压码流至YUV444p,最后转换成RGB565并显示
			JPEG文件头的格式主要包含：(详细的文件头见 fmt_jpeg.h 的结构体)
				SOI:	图像开始	label only
				APPn:	应用信息	JFIF/Exif/...
				DQT:	量化表		有损压缩的系数表
				SOF0:	结构开始	width/height, color space, yuv sample factor
				DHT:	Huffman表	无损压缩的范式Huffman表(树)
				SOS:	扫描开始	y/u/v分别对应的表号	<before compressed image data>
				EOI:	图像结束	label only			<after compressed image data>
  * @param  path: jpeg文件完整路径
  * @retval None
  ************************************************************************************************/
void ef_multimedia_pic_jpeg(char* path_t)
{
	oled_buffer_clear(0, 0, WIDTH-1, HEIGHT-1);
	f_res = f_open(&fp, path_t, FA_READ | FA_OPEN_EXISTING);
	if(f_res==FR_OK)
	{
		u16 imgw=0, imgh=0;
		u8 sample_t = 0;
		//预读取图片的宽度和高度
		rd_SOF0_2get_size(&fp, &sample_t, &imgw, &imgh);
		if(imgw > WIDTH || imgh > HEIGHT) {
			oled_show_string( 0, 0, (u8*)"picture too large", 12, BLACK, CYAN);
			return;
		}		
		if(sample_t != 0x11 && sample_t != 0x22) {
			oled_show_string( 0, 0, (u8*)"invalid sample_t", 12, BLACK, CYAN);
			return;
		}
		
		//u8* yuv_Y = (u8*)malloc(sizeof(u8) * 256);
		//u8* yuv_U = (u8*)malloc(sizeof(u8) * 64);
		//u8* yuv_V = (u8*)malloc(sizeof(u8) * 64);
		/*if(yuv_Y==NULL && yuv_U==NULL && yuv_V==NULL)
		{
			oled_show_string(0, 20, (u8*)"no heap size!", 12, CYAN, BLACK);
			oled_refresh();
			delay_ms(2000);
		}*/
		u8 yuv_Y[256] = { 0 };
		u8 yuv_U[64] = { 0 };
		u8 yuv_V[64] = { 0 };

		//根据长宽自动计算左上角的坐标(居中显示)
		u8	origin_x = (WIDTH - imgw) / 2;
		u8	origin_y = (HEIGHT - imgh) / 2;
		
		//此函数内部会更新好Frame_buf[]的数据
		Dec_JPEG_to_YuvRgb(&fp, yuv_Y, yuv_U, yuv_V, origin_x, origin_y);
		
		oled_refresh();

		//free(yuv_Y);
		//free(yuv_U);
		//free(yuv_V);
	}
	else
		oled_show_string(0, 0, (u8*)"File path error.", 12, BLACK, CYAN);
}


