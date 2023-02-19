#ifndef _FMT_BMP_H
#define _FMT_BMP_H

/* ==================================================================================================== */
/*											BMP ͼƬ��ʽ �ṹ��											*/
/*			Windowsɨ��BMP�����µ�λ��4Byte�����Ҫ��ÿ������Ҫ4�ֽڶ��룬����4�ֽ�������Ҫ��0			*/
/*			 ���㹫ʽΪ�� ÿ�в�0��ʵ���ֽ��� = ((ͼƬ��������� * ���ض�Ӧbit�� + 31)/32)*4			*/
/*	���磺ɫ��1bÿ��85����ʱÿ��85bit=��0��ÿ��12�ֽڣ�ɫ��16bÿ��85����ʱÿ��1360bit=��0��ÿ��172�ֽ�	*/
/* ---------------------------------------------------------------------------------------------------- */


typedef unsigned char 		u8;
typedef unsigned short int	u16;
typedef unsigned int 		u32;


struct s_BMP_header{			//14 Byte --- BMP�ļ�ͷ
	u16 Type;					//�ļ�����(��ȡBM,BA,CI,CP,IC,PT)
	u32 Size;					//�ļ���С(Byte)
	u16 Reserved1;				//����,����Ϊ0
	u16 Reserved2;				//����,����Ϊ0
	u32 Offbits;				//λͼ����ƫ�Ƶ�ַ(��ʹ�е�ɫ��Ҳ�ᱻƫ�ƹ�ȥ)
};

struct s_BMP_info_header{		//40 Byte --- BMP��Ϣͷ(����16b-565����Ϣͷ��С����56,Ҳ���ǰ�16Byte��ɫ��Ҳ���ȥ��)
	u32 biSize;					//info_header�ṹ��Ĵ�С(���õ�ֻ��40Byte,�ɰ����12,64,108,124)
	u32 biWidth;				//ͼƬ���(pixel)
	u32 biHeight;				//ͼƬ�߶�(����:�׵���; ����:������)(һ�㶼������)
	u16 biPlanes;				//��ɫƽ����(����1)
	u16 biBitCount;				//ɫ��(1,4,8,16,24,32)
	u32 biCompress;				//ͼƬѹ������(0:RGB(��ѹ��)(����); 1:RLE8(8λBMP); 2:RLE4(4λBMP);
								//3:BitFields(������,16/32λͼ); 4/5:BMP����JPG/PNG(�����ڴ�ӡ��))
	u32 biSizeImage;			//4�ֽڶ����ͼƬ��С
	u32 biXPelsPerMeter;		//ˮƽ�ֱ���(pixel/m)
	u32 biYPelsPerMeter;		//��ֱ�ֱ���(pixel/m)
	u32 biClrUsed;				//BMPʵ��ʹ�õĵ�ɫ��������Ŀ(�����Ϊ0,����ʹ�����е�ɫ������)
	u32 biClrImportant;			//��ͼƬ��ʾ����Ҫ�ĵ�ɫ��������Ŀ(�����Ϊ0,����������������Ҫ)
};

struct s_BMP_RGB_quad{			//24/32λBMPû�е�ɫ��,16λɫͼһ��Ҳû�е�ɫ��
	u8  rgbBlue;				//biBitCountָʾquad�ṹ��ĸ���(��=1������2��quad,��=8������256��quad)
	u8  rgbGreen;				//16λBMP��biCompression=0ʱ����û�е�ɫ��(quad),=3ʱquad����555��565���4��32bit������
	u8  rgbRed;					//555-RGB��16λBMPĬ�ϸ�ʽ(MSbit=0); 565-RGB����:0x00F800/0x0007E0/0x00001F(ע��С��ģʽ)
	u8  Alpha;					//Alpha��������͸��ͨ��ʱ����Ϊ0
};

struct s_BMP_565_MASK{			//�����ڱ�׼��BMP�ļ�ͷ
	u8  mask[16];				//���ڱ���RGB565������:0x00F800/0x0007E0/0x00001F/Alpha:0x0000(ע��С��ģʽ)
};



#endif
