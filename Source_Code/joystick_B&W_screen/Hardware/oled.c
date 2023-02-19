#include "delay.h"
#include "oled.h"
#include "oledfont.h"
#include "spi.h"
#include "dma.h"
#include <math.h>
#include "extfunc.h"

///////////////////////////////////////////////////////////////////////////Plexer//
// 1.������ʹ��ʵ�����ص�����, column(0~127)ӳ��ΪX��, row(0-63)ӳ��ΪY��
// 2.���ǵ���ʾ�������к�, (0,0)λ�����Ͻ�, ��X������ָ��, Y������ָ��
// 3.��Ϊoled_ssd1306ֻ��ҳд, һ��Ҫдһ��8bit = 1Byte
//   Ϊ�˷�ֹ����ʱ�����ı�Ե�ڸ�֮ǰ�ĺۼ�, ����������"���ݻ����"
// 4."�����"��������ˢ��ȫ����һ֡����, ���������һ��д��GRAM����ʾ
// 5.�ļ���������ר�ŵ�oled_refresh()����, �Դ���buffer�����ݵ�GRAM��
// 6.���к�����ֻ���޸�buffer����, ��ʹ���㺯��Ҳֻ�����buffer����
// 7.���ֻ���֡���ݵ�����������������, ֻҪ����ˢ��ÿһ֡(����fps)����
//////////////////////////////////////////////////////////////////////////2020/4/19
//OLED���Դ�GRAM��ʽ����:
// [0] 0 1 2 3 ... 127
// [1] 0 1 2 3 ... 127
// [2] 0 1 2 3 ... 127
// [3] 0 1 2 3 ... 127
// [4] 0 1 2 3 ... 127
// [5] 0 1 2 3 ... 127
// [6] 0 1 2 3 ... 127
// [7] 0 1 2 3 ... 127

//��fram_buffer����GRAM�Ĺ���������������spi��DMA��ʽ, ��������Ŷ
u8 Frame_Buffer[128][8];//fram_buffer��ÿһ��Ϊ8bit,һ��128*8��8bit


//ԭ�ӵİ����ϵ�PA.234�ֱ���Flash,SDcard,oled��CS��, PC.4��NRF��CS��.
//�ݶ�OLED_CS(PB0),	OLED_RST(PC4), OLED_DC(PC5), ��spi1���������߻���SPI1_Init()�еõ���ʼ��
void oled_spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
#if DMA_MODE
	DMA_InitTypeDef  DMA_InitStructure;		//DMA1 Structure
#endif	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;	//RST(PC4), DC(PC5)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);			//����
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//CS(PB0)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);						//����

#if DMA_MODE	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);			//ʹ��DMA����
	DMA_DeInit(DMA1_Channel3);									//����DMA1��ͨ��3(SPI1_TX)
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI1->DR;	//DMA�������ַ -> SPI1
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&Frame_Buffer;	//DMA�ڴ����ַ -> FrameBuffer
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;			//���ݴ��䷽��Ϊ����������(SPI)
	DMA_InitStructure.DMA_BufferSize = 128;						//DMAͨ����DMA����Ĵ�СΪ 128 Byte
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ�Ĵ���������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;	//���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;			//���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;				//��������ͨģʽ,��ѭ���ɼ�
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;			//DMAͨ��3�����ȼ�Ϊ �ϸ߼� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;				//DMAͨ��3 �����ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	//DMA_Cmd(DMA1_Channel3, ENABLE);//��oleda_refresh()������DMA, ���ж�Handler()�н�ֹDMA
#endif

	SPI1_Init();//��ʼ��SPI1,(������������ Baud = 4.5M )
}

//��SSD1306д��һ���ֽ�, cmd=0��ʾ����;  /1��ʾ����;
void oled_write_Byte(u8 dat,u8 cmd)
{
	OLED_DC = cmd; 			//ѡ�� data/cmd
	OLED_CS = 0;		 	//��spiƬѡ
	SPI1_ReadWriteByte(dat);//spi����
	OLED_CS=1;		  		//�ر�spiƬѡ
	OLED_DC=1;   	  		//Ĭ��ѡ�� data
}
	  	  
//����OLED��ʾ    
void oled_display_on(void)
{
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC����
	oled_write_Byte(0x14,OLED_CMD);  //DCDC ON
	oled_write_Byte(0xAF,OLED_CMD);  //DISPLAY ON
}

//�ر�OLED��ʾ     
void oled_display_off(void)
{
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC����
	oled_write_Byte(0x10,OLED_CMD);  //DCDC OFF
	oled_write_Byte(0xAE,OLED_CMD);  //DISPLAY OFF
}

//����buffer����������(����Ļ��ʱûӰ��,�������ˢ��)
/*
void buffer_clear(void)
{
	u8 x, y;		    
	for(y=0;y<8;y++)
		for(x=0;x<128;x++)
			Frame_Buffer[x][y] = 0x00;
}
*/
//������������Ǿ���, ��x0<x1,y0<y1
//��0�����ǰ���������
void oled_buffer_clear(u8 x0, u8 y0, u8 x1, u8 y1)
{
	u8 page0 = y0/8, offset0 = y0%8;
	u8 page1 = y1/8, offset1 = y1%8;
	u8 temp0 = 0xFF << offset0;			//��y0ƫ�ƴ����¶�Ӧ��0
	u8 temp1 = 0xFF << (offset1+1);		//��y1ƫ�ƴ����϶�Ӧ��0
	u8 x, y;		    

	if(page0 == page1)					//�����0�������´�����ͬpage��
	{
		if(offset0 == offset1)			//�����offsetҲһ��
		{
			temp0 = 0x01 << offset0;
			for(x=x0; x<x1+1; x++)		//�൱�ڻ�һ������
				Frame_Buffer[x][page0] &= ~temp0;
		}
		else							//������ͬpage�����ĳ����
		{
			temp0 = ~temp0 + temp1;		//�����е�����
			for(x=x0; x<x1+1; x++)		//�൱�ڻ�һ����ɫ����
				Frame_Buffer[x][page0] &= ~temp0;
		}
	}
	else								//�������ֻ��һ��page����
	{
		for(x=x0; x<x1+1; x++)			//�����page0����������
			Frame_Buffer[x][page0] &= ~temp0;
		y = page0 + 1;
		while(y < page1)
		{
			for(x=x0; x<x1+1; x++)
				Frame_Buffer[x][y] = 0x00;	//���е�pageһ�����Ϊ0
			y++;
		}
		for(x=x0; x<x1+1; x++)			//��������page1����������
			Frame_Buffer[x][page1] &= temp1;
	}
}

//��buffer����������GRAM ��ˢ��һ֡��Ļͼ��
void oled_refresh(void)
{
	u8 i,n;		    
	for(i=0;i<8;i++)  
	{  
		oled_write_Byte(0xb0+i,OLED_CMD);    //����ҳ��ַ(0~7)
		oled_write_Byte(0x00,OLED_CMD);      //������ �͵�ַ
		oled_write_Byte(0x10,OLED_CMD);      //������ �ߵ�ַ

#if DMA_MODE		
		DMA_ClearFlag(DMA1_FLAG_TC3);
		DMA_ITConfig(DMA1_Channel3, DMA_IT_TC, ENABLE);//����DMA_TC�жϱ�־
		
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		/*!< DMA1 Channel3 enable */
		DMA_Cmd(DMA1_Channel3, ENABLE);//��oleda_refresh()������DMA, ���ж�Handler()�н�ֹDMA
		
		OLED_DC = OLED_DATA;//ѡ��data/cmd
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
			oled_write_Byte(Frame_Buffer[n][i],OLED_DATA);//LSBitʵ����������һ���ŵ�����
#endif
	}
}


//����
void oled_clear(void)
{
	oled_buffer_clear(0, 0, 127, 63);//Ĭ����0ȫ��
	oled_refresh();
}

//���, x(0~127),y(0~63)�����ص�����,ע��(0,0)���� ���Ͻ�
//mode:1 (��GRAM��)�������ص�    0: (��GRAM��)Ϩ�����ص�
void oled_draw_point(u8 x, u8 y, u8 mode)
{
	u8 page, offset;
	u8 temp=0;
	if(x>127 || y>63) return;	//������Χ
	page = y/8;					//���ڵ�ҳ
	offset = y%8;				//ҳ��ƫ��
	temp = 0x01 << offset;
	
	//��Ϊ����buffer����, ������Ҫlocate()����ѡ������!
	if(mode)	Frame_Buffer[x][page] |= temp;
	else		Frame_Buffer[x][page] &= ~temp;
}

//������������ʾ�ַ�,lib֧��0806,1206,1608
//x(0~127),y(0~63)     /mode:0,������ʾ(��������)   1,������ʾ
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u8 mode)
{      			    
	u8 temp,i,k;
	u8 y0 = y, x0 = x;	//��¼ԭʼy��x
	chr = chr - ' ';	//�õ�ƫ�ƺ��ֵ(���ڵ�����)
	if(lib==8)			//0806
	{
		for(i=0; i<6; i++)
		{
			if((x>=128)||(y>=64))	continue;		//����ʾ��ûԽ��Ĳ���(�������ݶ��ź;��)����Ҳ�Խ��, ������
				
			temp = ascii_0806[chr][i];
			for(k=0; k<8; k++)				//ÿ����8����
			{
				if(temp&0x01)	oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
				else 			oled_draw_point(x,y,!mode);
				temp >>= 1;
				y++;			//׼�������·���һ��Ԫ�ص�
			}
			y = y0; x++;					//�������Ҳ�һ�еĵ�
		}
	}
	else if(lib==12 || lib==16)	//1206,1608
	{
		u8 csize=( lib/8+((lib%8)?1:0) ) * (lib/2);	//һ���ַ�ռ���ֽ���,1206Ϊ12,1608Ϊ16
		for(i=0;i < csize;i++)
		{
			if((x>128)||(y>=64))	continue;		//����ʾ��ûԽ��Ĳ���(�������ݶ��ź;��)����Ҳ�Խ��, ������
			if(lib==12)		temp = ascii_1206[chr][i];
			else			temp = ascii_1608[chr][i];
			if(i < csize/2)		//1206�״� ��8��; 1608ÿ����8��
			{
				for(k=0; k<8; k++)
				{
					if(temp&0x01)	oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
					else			oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//׼�������·���һ��Ԫ�ص�
				}
				y = y0; x++;						//�������Ҳ�һ�еĵ�
			}
			else				//1206�ڶ��� ��4��; 1608ÿ����8��
			{
				y = y0 + 8;
				x = x0 + i - csize/2;	//��λ����ʼ�ڶ���ɨ���������
				for(k=0; k<8; k++)
				{
					if(temp&0x01)	oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
					else			oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//׼�������·���һ��Ԫ�ص�
					if((y-y0)==lib)					//���1206�赽��12����,�򼴿̻�����һ��
					{
						y = y0+8;
						x++;
						break;						//���1206�����,������һ�м������
					}
				}
			}
		}
	}
	else
		return;//�޶�Ӧ���ַ���
}

//m^n���ݺ���,���ļ����ڲ�����
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)
		result *= m;    
	return result;
}

//��ʾһ������(��λ��0����ʾ)
//x,y:����	  /len:��λ��   /lib:����   /mode:0,������ʾ  1,������ʾ			  
void oled_show_num_hide0(u8 x, u8 y, u32 num, u8 len, u8 lib, u8 mode)
{         	
	u8 temp;
	u8 enshow=0;
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//�������ҵõ�ÿһλ����

//		
		if(temp==0 && t==len-1)				//������һ������0,����ʾ'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==0)	//���ǰ��λ�г���0,��Ŀǰ��δ���ַ�����,����ʾ' '
		{
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,' ',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==1)	//���ǰ��λ�г���0,���������ֹ�������,����ʾ'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else								//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
		{
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
			else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		}
//		
		/*		
		if(enshow==0 && t<(len-1))			//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
		{
			if(temp==0)
			{
				if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);//0806��Ϊ2+8/2=6
				else		oled_show_char(x + t*(lib/2),y,' ',lib,mode);//1206,1608���Ǹߵ�1/2
				continue;//continue��������һ��ѭ��
			}
			else enshow=1;	 	 
		}
		else ;
		
	 	if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
		else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		*/
	}
}
//��ʾһ������(��λ��0��Ȼ��ʾ)    /mode:0,������ʾ  1,������ʾ
void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u8 mode)
{         	
	u8 temp;						   
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//�������ҵõ�ÿһλ����
		if(lib==8)	oled_show_char(x + t*6,y,temp+'0',lib,mode);//0806��Ϊ2+8/2=6
		else		oled_show_char(x + t*(lib/2),y,temp+'0',lib,mode);//1206,1608���Ǹߵ�1/2
	}
}

/**
  * @brief  ��ʾС��(�����ɸ�)
  * @note	ע�⸺�Ż�ռ�õ�1��Ŀռ�, Ĭ�ϲ���ʾ����
  * @param  fnum: ��Ҫ��ʾ��С��(float)
  *			len1: ���������ֵĳ���, ע�⸺������Ϊ���Ŷ�������ʾ1��
  *			len2: ��С�����ֵĳ���, ����������λС��
 *			mode: 0,������ʾ  1,������ʾ
  * @retval None
  */
void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u8 mode)
{
	u32 integer = 0;	//���յ�С���������������
	u16 pwr = 1;		//�������ʵ��С���Ĵ��ݱ���(1,10,100,1000, ...)
	u8 len22 = len2;	//��Ϊlen2���·����Լ�, ����Ҫ���Ᵽ��len2�ĳ�ֵ
	while(len2)
	{
		pwr *= 10;
		len2--;
	}
	if(fnum < 0)//����
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, '-', lib, mode);//�������������1����ʾ����
		
		fnum = fabs(fnum);
		
		integer = fnum;	//����������ֵ�ֵ
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, mode);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, mode);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//���С�����ֵ�ֵ
		oled_show_num_every0(x, y, integer, len22, lib, mode);
	}
	else		//����
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, ' ', lib, mode);//�������������1����ʾ�ո񷽱㸲�ǵ�����
		
		integer = fnum;	//����������ֵ�ֵ
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, mode);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, mode);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//���С�����ֵ�ֵ
		oled_show_num_every0(x, y, integer, len22, lib, mode);
	}
}


/**
  * @brief  ����2Byte��GBK���� -> ��SD������������Ϣmatrix -> ��ʾ
  * @note	FatFs֧��GBK����, ���ǳ��ļ���������Unicode�ַ�����UTF-16��ʽ
  *			����FatFs��ĵ�cc936.c��Unicode��GBK�໥ת��
  *			��������������GBK��(2Byte), ���ڲ���ȡSD��Ѱ�ҵ�������matrix[]
  *			��ע���������´�ֱɨ�赽��, ��λ��ǰ
  * @param  x, y: ����
  *			gbk : 16bit GBK ��ֵ
  *			lib : �����С
 *			mode: 0,������ʾ  1,������ʾ
  * @retval None
  */
void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u8 mode)
{
	u8 temp, i, k;
	u8 y0 = y;
	u8 matrix[72] = {0};	//֧������ֺ�24 -> 24*24/8 = 72 Byte
	u8 csize = (lib/8 + ((lib%8)?1:0)) * lib;	//12->24Byte, 16->32Byte, 24->72Byte
	if((lib!=12)&&(lib!=16)&&(lib!=24))
		return;		//���������ֺŲ�ƥ��, ����ʾ
	else
	{
		//��ȡSD�� �õ���Ӧ�ֺ� ��Ӧ����� ��������
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
					break;	//�������һ����������һ�еĻ���
				}
			}
		}
	}
	
	
}


//��ʾ�ַ���
//x,y:�������    /lib:�����С    /mode:0,������ʾ  1,������ʾ
void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u8 mode)
{	
	u8 i=0;
	u8 x0=x;
	while(p[i] != '\0')
	{		
		if(p[i]=='\r')							//�������'\r'��
		{	
//			oled_show_char(x,y,' ',lib,mode);	//������� ���� ��ʾһ��' '
			i++;
			continue;							//�����·������ٽ���whileѭ����ֹ����ƫ����ɼ��һ���ַ����
		}
		else if(p[i]=='\n')						//���������'\n'��
		{
//			oled_show_char(x,y,'\\',lib,mode);	//��ʾһ��slash
			//oled_show_char(x,y,'n',lib,mode);	//��ʾһ��'n'
			x = (lib==16)?0:2;					//������ʾ x+2 or x+0
			y = y+lib+1;						//������ʾ y+1
			i++;
			continue;							//�����·������ٽ���whileѭ��
		}
		else if(p[i]>=0x81)	//���������
		{
			if(x>=128-lib/2+3)			//���ʣ���������һ�뺺�ֶ�������ʾ����
			{
				x = x0;
				y = y + lib + 1;
			}
			else if(x>=128-lib+1)
			{
				//���ֻʣ������ֵĿռ�, ֻ������ʾ���������ĸ�����, ���඼Ӧ�ü��̻���
				if(((p[i]!=0xA1)&&(p[i+1]!=0xA3))/* ��*/ || ((p[i]!=0xA3)&&(p[i]!=0xAC))/* ��*/ \
						|| ((p[i]!=0xA3)&&(p[i+1]!=0xBA))/* ��*/ || ((p[i]!=0xA3)&&(p[i+1]!=0xBB))/* ��*/)
				{
					x = x0;
					y = y + lib + 1;
				}
				else ;
			}
			else ;
			if((p[i]==0xFF)||(p[i+1]<0x40)||(p[i+1]==0x7F)||(p[i+1]==0xFF))//�������GBK���뷶Χ���򷵻�
			{
				i += 2;//��Ϊ������2Byte, ����iӦ�ö��Լ�һ��
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
		
		/* ���ѳ�����˵������string�����Ҳ�߽�, ���м�����ʾ 0806/1206:x+2, 1608:x+0, y+1 */
		/* ��string���Ҳ����߽粻�㹻��ʾ��һ���ַ�, ������ʾ */
		/* ����Ҫע����� ��� �� ���� ��ʵ����ģ�������, �Ͼ����ǲ�̫���ʻ�����ʾ */
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
  * @brief  ��ʾ������ͼƬ(*.bin), �˺���Ŀǰû��������, ��ʱ������ĳ����������ʾ������ݱȵ�binͼƬ
  * @note	ͼƬ��С85*64, �������´�ֱɨ�赽��, ��λ��ǰ, ע���ֿ�ȫ0Ϊ�ո�, ͼƬȫ0Ϊ��ɫͼƬ
  *			Ҳ��ͼƬȫ��ʱwinhex��ʾΪȫ1 = ȫ��0XFF, ������oled��ɫ������������(����0)
  *			(64/8)*85 = 680Byte
  * @param  x, y: 	�������
  *		   *pPic:	ͼƬ��������Ϣ, ����Ƶ��һ֡��Ϣ
  *			mode:  	/1,�������ص�(�Ƽ�����1)    /0,Ϩ�����ص�
  * @retval None
  */
void oled_show_BinImage(u8 x, u8 y, const char *pPic, u8 mode)//3��badapple��bin��ʽ: 085*64, 106*80, 170*128
{         	
	u8 temp;
	u8 y0 = y;
	for(u16 i=0; i<680; i++)	//��ѡ 680, 1060, 2720
	{
		temp = pPic[i];
		for(u8 j=0;j<8;j++)
		{
			if(temp & 0x80)
				oled_draw_point(x, y, !mode);	//��λ��1�������ص�ԭΪ��ɫ-> Ϩ�����ص�-> !mode=0
			else
				oled_draw_point(x, y, mode);
			temp <<= 1;
			y++;							//�������·������ص�				
			if((y - y0)== 64)	//��ѡ 64, 80, 128
			{
				y = y0;
				x++;
				if(x>=128) return;			//��������(��ֱɨ��ʱ����Խ�����return)
				else;
				break;
			}
			else;
			if(y>=64) break;				//��������(��ֱɨ��ʱ����Խ�����break)
			else;
		}
	}
}

/**
  * @brief  ��ʾ������ͼƬ(*.bin)������ͼƬ(*.c), �˺�����ȫ���Բ��������Image����
  * @note	�˺�����ʱ������ʾ1:1��Сͼ��, �ݶ�������ʽ(48*48, 32*32)
  *			ͼƬȫ��ʱwinhexΪȫ1, ��Ӧ����oled����ȫ0���ر���ʾ�ﵽ��ɫ��Ч��
  * @param  x, y: 	�������
  *		    Icon:	ͼ�����(��oled.h��define����), ָ��ͼƬ��������Ϣ, ����Ƶ��һ֡��Ϣ
  *			mode:  	/1,�������ص�(�Ƽ�����1)    /0,Ϩ�����ص�
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
				oled_draw_point(x,y,!mode);	//��λ��1�������ص�ԭΪ��ɫ-> Ϩ�����ص�-> !mode=0
			else
				oled_draw_point(x,y,mode);
			temp <<= 1;
			y++;							//�������·������ص�				
			if((y - y0)== height)
			{
				y = y0;
				x++;
				if(x>=128) return;			//��������(��ֱɨ��ʱ����Խ�����return)
				else;
				break;
			}
			else;
			if(y>=64) break;				//��������(��ֱɨ��ʱ����Խ�����break)
			else;
		}
	}
}



//��ʼ��SSD1306					    
void oled_ssd1306_init(void)//Ĭ�ϲ����� (Page_0,Column_0)��oled���Ͻǵ�λ��
{
	OLED_RST = 1;
	delay_ms(45);
	OLED_RST = 0;//�͵�ƽ��Ч
	delay_ms(120);
	OLED_RST = 1;
	delay_ms(5);
	
	oled_write_Byte(0xAE,OLED_CMD);	//�ر���ʾ
	oled_write_Byte(0x00,OLED_CMD);	//�����еĵ͵�ַ
	oled_write_Byte(0x10,OLED_CMD);	//�����еĸߵ�ַ
	oled_write_Byte(0x40,OLED_CMD); //������ʾ��ʼ��[5:0],����.
	//oled_write_Byte(0xB0,OLED_CMD);//����ҳ�׵�ַ
	
	oled_write_Byte(0x81,OLED_CMD); //�Աȶ�����
	oled_write_Byte(0x4F,OLED_CMD); //1~255; Ĭ��0X7F(��������,Խ��Խ��)
	oled_write_Byte(0xA1,OLED_CMD); //0xA1Ĭ�ϲ���,0xA0���ҷ���
	oled_write_Byte(0xC8,OLED_CMD); //0xC8Ĭ�ϲ���,0xC0���·���
	
	oled_write_Byte(0xA6,OLED_CMD); //0xA6ȫ����ɫ��ʾ,0xA7ȫ�ַ�ɫ��ʾ
	oled_write_Byte(0xA8,OLED_CMD); //��������·��(1 to 64)
	oled_write_Byte(0X3F,OLED_CMD); //Ĭ��0x3F(1/64)
	oled_write_Byte(0xD3,OLED_CMD); //������ʾƫ��
	oled_write_Byte(0X00,OLED_CMD); //Ĭ��ƫ��Ϊ0
	oled_write_Byte(0xD5,OLED_CMD); //����ʱ�ӷ�Ƶ����,��Ƶ��
	oled_write_Byte(0x80,OLED_CMD); //[3:0],��Ƶ����; [7:4],��Ƶ��
	//oled_write_Byte(0xD8);//*** -set area color mode off
	//oled_write_Byte(0x05);//***
	oled_write_Byte(0xD9,OLED_CMD); //����Ԥ�������
	oled_write_Byte(0xF1,OLED_CMD); //[3:0],PHASE 1 ; [7:4],PHASE 2
	oled_write_Byte(0xDA,OLED_CMD); //����COMӲ����������
	oled_write_Byte(0x12,OLED_CMD); //[5:4]����
	
	
	oled_write_Byte(0xDB,OLED_CMD); //����VCOM ��ѹ����	
	oled_write_Byte(0x00,OLED_CMD); //0x00Ϊ0.65*VCC; 0x20Ϊ0.77*VCC; 0x30Ϊ0.83*VCC;(��ѹԽ��Խ��)
	oled_write_Byte(0x20,OLED_CMD); //�����ڴ��ַģʽ 
	oled_write_Byte(0x02,OLED_CMD); //0x00ˮƽѰַ,0x01��ֱѰַ,0x02ҳѰַ(Ĭ��)
	oled_write_Byte(0x8D,OLED_CMD); //��ɱ�����
	oled_write_Byte(0x14,OLED_CMD); //�õĿ���/�ر�
	oled_write_Byte(0xA4,OLED_CMD); //0xA4Ϊ��ʾ�ڴ������,0xA5�����ڴ������
	oled_write_Byte(0xAF,OLED_CMD); //������ʾ
	oled_clear();
	//oled_buffer_clear(0, 0, 127, 63);
}  



//Plexer�����Ѿ���֤���������д������ȷ��.
