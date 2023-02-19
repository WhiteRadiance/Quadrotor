//#include "extfunc.h"
#include "PictureLib.h"
#include "ff.h"
//#include "sdio_sdcard.h"
#include "oled.h"
#include "delay.h"
//#include <string.h>
//#include <stdio.h>
#include <stdlib.h>			//malloc
/*��˵������
/  ��c�ļ����� BMP,PNG,JPG ͼƬ��ʽ����ʾ��������չextfunc.c�еĶ�ý��ͼƬ�鿴��	*
/  ���ĵĺ������ֱ���Բ�ͬ��ͼƬ��ʽ������������ʽ��ef_multimedia_pic_<FORMAT>		*/

/**
  *************************************************************************************************
  *											�������
  *************************************************************************************************
  */
extern FRESULT	f_res;			//File function return code (FRESULT), ������extfunc.c
extern FATFS 	fs;				//FatFs�ļ�ϵͳ�ṹ��, ������extfunc.c
extern FIL		fp;				//�ļ��ṹ��ָ��, ������extfunc.c
extern UINT		bw,br;			//��ǰ�ܹ�д��/��ȡ���ֽ���(����f_read(),f_write())
extern FILINFO	fno;			//�ļ���Ϣ, ������extfunc.c


#if defined SSD1306_C128_R64
	extern u8	Frame_Buffer[128][8];
#elif defined SSD1351_C128_R128
	extern u16	Frame_Buffer[128][128];
#elif defined SSD1351_C128_R96
	extern u16	Frame_Buffer[96][128];
#endif


/**
  *************************************************************************************************
  *											���庯��
  *************************************************************************************************
  */

/**************************************************************************************************
  * @brief  ��bmp������bufȥ����Frame[]����
  * @note	����������ɨ��,�������������,���ӵײ�������
			�����ڵ�ɫbmp,�����ڴ�c�ļ��ڲ�ʹ��
  * @param  ������oled_show_Bin_Array()
  * @retval None
  ************************************************************************************************/
/*
static void PicLib_show_bmp_buf(u8 x, u8 y, const u8 *pPic, u8 width, u8 height, u8 depth)
{
	if(depth != 0x01)
		return;
	u16 n = 0;
	u8  temp;
	for(u8 j=1; j<=height; j++){								//�����е���ɨ��(���͵�BMP��ʽ)
		for(u8 i=0; i<width; i++){
			temp = 0x80;
			for(u8 t=0; t<8; t++){
				if(pPic[n] & temp){								//��ȡbin�õ���0xFF�����ɫ
					Frame_Buffer[height-j+y][i*8+x+t] = BLACK;
					//Frame_Buffer[j+y][(i*8+x+t)*2+1] = (u8)BLACK;
				}
				else{
					Frame_Buffer[height-j+y][i*8+x+t] = CYAN;
					//Frame_Buffer[j+y][(i*8+t+x)*2+1] = (u8)CYAN;
				}
				if(i*8+x+t+1 >= WIDTH)
					break;
				if(i*8+t+1 == width)							//�����ʾ��ͼƬ�����, ��0������������ʾ
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
  * @brief  ͼƬ�鿴����Ͻ��bmpͼƬ�鿴��
  * @note	�����ȶ�ȡ�ļ���ʽ��Ϣ��Ȼ����ʾBMP�е�ʵ������
			BMP�ļ�ͷ�ĸ�ʽ��Ҫ������(��ϸ���ļ�ͷ�� PictureLib.h �Ľṹ��)
				BMP_Header:		�ļ�ͷ(14Byte)	BM��ǩ,�ļ���С,ƫ�Ƶ�ַ...
				BMP_InfoHeader: ��Ϣͷ(40Byte)	ͼƬ�ߴ�,ɫ��,ɫ��bit��,��ɫ��������...
				BMP_RGB_quad:	��ɫ��(��ѡ)	�����Ŷ�Ӧĳ����ɫ�����ɫֵ
				BMP_ColorData:	ͼƬ����		����ɫֵ(���ɫ������)��ɵ���Ϣ
  * @param  path: bin�ļ�����·��
  * @retval None
  ************************************************************************************************/
void ef_multimedia_pic_bmp(char* path_t)
{
	u8  bmp_head[54]={0};				//����54Byte�ļ�ͷ(info_head����40Byte���жϳ���)
//	u8 	pic_width=0, pic_height=0;		//ͼƬ�ĳ���
//	u8 	pic_color_depth=0;				//ͼƬɫ�����(Ŀǰֻ�жϵ�ɫ��16bit���)

	u8	origin_x=0, origin_y=0;			//���ݳ����Զ��������Ͻǵ�����(������ʾ)
	u8	pic_buf[2048];					//128*128�ĵ�ɫͼƬֻ����128*128/8=2048Byte(����ɫͼƬ������ȫװ����)
	
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

		if(Header.Type != 0x424D){																	//û�з���"BM"��ǩ
			oled_show_string( 0, 0, (u8*)"No 'BM' tag", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
//		if(InfoHdr.biSize != 0x00000028){															//BMP_INFO_HEADER�Ĵ�С������40Byte
//			oled_show_string( 0, 0, (u8*)"InfoHead != 40", 12, BLACK, CYAN);						//����16b-565�Ĵ�С��ʱ��56Byte
//			f_close(&fp);																			//Ϊ�˼����Կ���,�˶���ע�͵�
//			return;
//		}
		if((InfoHdr.biWidth>WIDTH) || (InfoHdr.biHeight>HEIGHT)){									//����֧���²�����ͼƬ�ߴ�̫��ᱨ��
			oled_show_string( 0, 0, (u8*)"picture too large", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
		if((InfoHdr.biBitCount!=0x0001) && (InfoHdr.biBitCount!=16) && (InfoHdr.biBitCount!=24)){	//ֻ֧��1b,16b,24bɫ�����
			oled_show_string( 0, 0, (u8*)"unsupported depth", 12, BLACK, CYAN);
			f_close(&fp);
			return;
		}
		origin_x = (WIDTH  - InfoHdr.biWidth)/2;													//ȷ��������ʾ�����Ͻ������
		origin_y = (HEIGHT - InfoHdr.biHeight)/2;
		
		u16 Raw_TotalByte_with0 = 0;																//bmp��0��ÿ��ռ���ֽ���
		u16 Raw_Extra_zerosByte	= 0;																//bmpÿ�б����˶����ֽ�0
		
		if(InfoHdr.biBitCount == 0x0001)			// =================================���bmp��ɫ��Ϊ 1bit ==================================
		{
			u8* qd = (u8*)malloc(sizeof(u8) * 8);													//malloc
			struct s_BMP_RGB_quad quad[2];															//�洢��ɫ��
			f_read(&fp, qd, 8, &br);
			quad[0].rgbRed	= qd[0];		quad[1].rgbRed	= qd[4];
			quad[0].rgbGreen= qd[1];		quad[1].rgbGreen= qd[5];
			quad[0].rgbBlue	= qd[2];		quad[1].rgbBlue	= qd[6];
			quad[0].Alpha	= qd[3];		quad[1].Alpha	= qd[7];
			free(qd);																				//free
			qd = NULL;			
			
			Raw_TotalByte_with0 = ((InfoHdr.biWidth * InfoHdr.biBitCount + 31) >> 5) << 2;			//�������bmpͼ��ÿ��ռ���ֽ���
			Raw_Extra_zerosByte = Raw_TotalByte_with0 - ((InfoHdr.biWidth+7) >> 3);					//������ݴ�ʱӦ�˵������ֽڵ�0
				
			for(u8 i=0; i<InfoHdr.biHeight; i++){													//��ɨ���ȡ, (��ָ��)�˵�������0
				f_res = f_read(&fp, pic_buf+i*(Raw_TotalByte_with0-Raw_Extra_zerosByte), Raw_TotalByte_with0, &br);
				br += br;
			}
			if(f_res==FR_OK){
				//1bitɫ��ĵ�ɫ��ֻ��������ɫ,����Ӧ0��1������(��������������1��0xFF��ָ��1������)
				//�����,�жϵ�0��������quad_Red=0xFF����Ϊ0�����������ɫ(��Ȼ����RGB����0xFF���ǰ�ɫ)
				//��ô,��һ����ɫ1����������Ȼ��ӦRGB����0x00,����ɫ
				if(quad[0].rgbRed == 0x00){
					for(u16 t=0; t<InfoHdr.biHeight*(Raw_TotalByte_with0-Raw_Extra_zerosByte); t++)	//��֤pic_buf���1�����ɫ����
						pic_buf[t] = ~pic_buf[t];
				}
				else ;
				
				//PicLib_show_bmp_buf(origin_x, origin_y, pic_buf, InfoHdr.biWidth, InfoHdr.biHeight, InfoHdr.biBitCount);
				
				
				
				
				
				
				
				
				u16 n = 0;
				u8  temp;
				for(u8 j=1; j<=InfoHdr.biHeight; j++){								//�����е���ɨ��(���͵�BMP��ʽ)
					for(u8 i=0; i<(InfoHdr.biWidth+7)>>3; i++){
						temp = 0x80;
						for(u8 t=0; t<8; t++){
							
							if(i*8+origin_x+t >= WIDTH)		break;
							if(j+origin_y >= HEIGHT)		break;
							
							if(pic_buf[n] & temp){									//��ȡbin�õ���0xFF�����ɫ
								Frame_Buffer[InfoHdr.biHeight-j+origin_y][i*8+origin_x+t] = BLACK;
								//Frame_Buffer[j+y][(i*8+x+t)*2+1] = (u8)BLACK;
							}
							else{
								Frame_Buffer[InfoHdr.biHeight-j+origin_y][i*8+origin_x+t] = CYAN;
								//Frame_Buffer[j+y][(i*8+t+x)*2+1] = (u8)CYAN;
							}

							if(i*8+t+1 == InfoHdr.biWidth)							//�����ʾ��ͼƬ�����, ��0������������ʾ
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
		else if(InfoHdr.biBitCount == 16)			// ================================= ���bmp��ɫ��Ϊ 16bit =================================
		{
			u16 n = 0;
			
			Raw_TotalByte_with0 = ((InfoHdr.biWidth * InfoHdr.biBitCount + 31) >> 5) << 2;			//�������bmpͼ��ÿ��ռ���ֽ���
//			Raw_Extra_zerosByte = Raw_TotalByte_with0 - InfoHdr.biWidth*2;							//������ݴ�ʱӦ�˵������ֽڵ�0
			
			if(InfoHdr.biCompress == 0)				// ================================= RGB-555(Default) =====
			{
				for(u8 i=1; i<=InfoHdr.biHeight; i++){												//��ɨ���ȡ,ֱ�Ӵ浽Frame[]��(���Ժ��Բ�0)
					f_res = f_read(&fp, pic_buf, Raw_TotalByte_with0, &br);
					n = 0;
					for(u8 j=0; j<InfoHdr.biWidth; j++){
						
						if(j+origin_x >= WIDTH)		break;											//�ӵײ���ʼ��ʾ,ע�����
						if(i+origin_y >= HEIGHT)	break;
						
						//RGB555 -> RGB565 (G��ĩλ��1��׷��1, G��ĩλ��0��ֻ��λ)
						u8  val_RG = pic_buf[n+1];													//16bit�а���R,Gɫ���Ǹ��ֽ�
						u8  val_GB = pic_buf[n];													//16bit�а���G,Bɫ���Ǹ��ֽ�
						u8  temp = val_GB & 0x1F;													//��ʱ����5bit�� Blue �ɷ�
						val_RG <<= 1;																//ת������ʾ��֧�ֵ�RGB-565��ʽ����ɫ
						val_RG |= val_GB>>7;
						if(val_GB & 0x20)	val_GB = ((val_GB&0xE0)<<1) + 0x20 + temp;
						else				val_GB = ((val_GB&0xE0)<<1) + temp;
																									//ע��RGB��ʽ(bmp order: B-G-R)
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
				f_read(&fp, RGB565.mask, 16, &br);													//��ȡR,G,B,Alpha������
				u32 MASK_R = RGB565.mask[ 0] + ((u16)RGB565.mask[ 1]<<8) + ((u32)RGB565.mask[ 2]<<16) + ((u32)RGB565.mask[ 3]<<24);
				u32 MASK_G = RGB565.mask[ 4] + ((u16)RGB565.mask[ 5]<<8) + ((u32)RGB565.mask[ 6]<<16) + ((u32)RGB565.mask[ 7]<<24);
				u32 MASK_B = RGB565.mask[ 8] + ((u16)RGB565.mask[ 9]<<8) + ((u32)RGB565.mask[10]<<16) + ((u32)RGB565.mask[11]<<24);
				u32 MASK_A = RGB565.mask[12] + ((u16)RGB565.mask[13]<<8) + ((u32)RGB565.mask[14]<<16) + ((u32)RGB565.mask[15]<<24);
				if((MASK_R!=0x0000F800) && (MASK_G!=0x000007E0) && (MASK_B!=0x0000001F) && (MASK_A!=0x00000000)){
					oled_show_string( 0, 26, (u8*)"Unknown 565 Mask", 12, BLACK, CYAN);
					f_close(&fp);
					return;
				}
				
				for(u8 i=1; i<=InfoHdr.biHeight; i++){												//��ɨ���ȡ, (��ָ��)�˵�������0
					f_res = f_read(&fp, pic_buf, Raw_TotalByte_with0, &br);
					n = 0;
					for(u8 j=0; j<InfoHdr.biWidth; j++){
						
						if(j+origin_x >= WIDTH)		break;											//�ӵײ���ʼ��ʾ,ע�����
						if(i+origin_y >= HEIGHT)	break;
																									//ע�������ȡ(bmp order: B-G-R)
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
		else if(InfoHdr.biBitCount == 24)		// ================================= ���bmp��ɫ��Ϊ 24bit =================================
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
  * @brief  ͼƬ�鿴����Ͻ��jpegͼƬ�鿴��
  * @note	����ƾ��fmt_jpeg.c���ļ�ͷ���н���,Ȼ���ѹ������YUV444p,���ת����RGB565����ʾ
			JPEG�ļ�ͷ�ĸ�ʽ��Ҫ������(��ϸ���ļ�ͷ�� fmt_jpeg.h �Ľṹ��)
				SOI:	ͼ��ʼ	label only
				APPn:	Ӧ����Ϣ	JFIF/Exif/...
				DQT:	������		����ѹ����ϵ����
				SOF0:	�ṹ��ʼ	width/height, color space, yuv sample factor
				DHT:	Huffman��	����ѹ���ķ�ʽHuffman��(��)
				SOS:	ɨ�迪ʼ	y/u/v�ֱ��Ӧ�ı��	<before compressed image data>
				EOI:	ͼ�����	label only			<after compressed image data>
  * @param  path: jpeg�ļ�����·��
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
		//Ԥ��ȡͼƬ�Ŀ�Ⱥ͸߶�
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

		//���ݳ����Զ��������Ͻǵ�����(������ʾ)
		u8	origin_x = (WIDTH - imgw) / 2;
		u8	origin_y = (HEIGHT - imgh) / 2;
		
		//�˺����ڲ�����º�Frame_buf[]������
		Dec_JPEG_to_YuvRgb(&fp, yuv_Y, yuv_U, yuv_V, origin_x, origin_y);
		
		oled_refresh();

		//free(yuv_Y);
		//free(yuv_U);
		//free(yuv_V);
	}
	else
		oled_show_string(0, 0, (u8*)"File path error.", 12, BLACK, CYAN);
}


