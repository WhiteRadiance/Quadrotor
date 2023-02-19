#include "delay.h"
#include "oled.h"
#include "oledfont.h"
#include "spi.h"
#include "dma.h"
#include <math.h>
#include "extfunc.h"

/////////////////////////////////////////////////////////////////////////////////////Plexer//
// 1. ������ʹ��ʵ�����ص�����, column(0~127)ӳ��ΪX��, row(0-63)ӳ��ΪY��
// 2. ���ǵ���ʾ�������к�, (0,0)λ�����Ͻ�, ��X������ָ��, Y������ָ��
// 3. ��Ϊoled_ssd1306ֻ��ҳд, һ��Ҫдһ��8bit = 1Byte
//    Ϊ�˷�ֹ����ʱ�����ı�Ե�ڸ�֮ǰ�ĺۼ�, ����������"���ݻ����"
// 4. "�����"��������ˢ��ȫ����һ֡����, ���������һ��д��GRAM����ʾ
// 5. �ļ���������ר�ŵ�oled_refresh()����, �Դ���buffer�����ݵ�GRAM��
// 6. ���к�����ֻ���޸�buffer����, ��ʹ���㺯��Ҳֻ�����buffer����
// 7. ���ֻ���֡���ݵ�����������������, ֻҪ����ˢ��ÿһ֡(����fps)����
// 8. OLED_IC����/��ɨ������Buffer, �������д��Bufferʱ��ȡģ�����M/LSb�Ƕ�����
/////////////////////////////////////////////////////////////////////////////////////2020/12/04
//OLED���Դ�GRAM��ʽ����:
	 /* SSD13006 */							/* SSD1351 */
// [0] 0 1 2 3 ... 127			[0]  RGB0 RGB1 RGB2 RGB3 ... RGB127	--- (RGB565 = 16bit= 2Byte)
// [1] 0 1 2 3 ... 127			[1]  RGB0 RGB1 RGB2 RGB3 ... RGB127
// [2] 0 1 2 3 ... 127			[2]  RGB0 RGB1 RGB2 RGB3 ... RGB127
// [3] 0 1 2 3 ... 127			...
// [4] 0 1 2 3 ... 127			...
// [5] 0 1 2 3 ... 127			...
// [6] 0 1 2 3 ... 127			[94] RGB0 RGB1 RGB2 RGB3 ... RGB127
// [7] 0 1 2 3 ... 127			[95] RGB0 RGB1 RGB2 RGB3 ... RGB127

//��fram_buffer����GRAM�Ĺ���������������spi��DMA��ʽ, ��������Ŷ
//���spi.h�ĵ��ߴ���DMA�Ƿ���һ
#if	SPI1_1Line_TxOnly_DMA
	#define DMA_MODE	1		//1: init��refresh����ΪDMA��ʽ
#else
	#define DMA_MODE	0
#endif


#if defined SSD1306_C128_R64
	u8	Frame_Buffer[128][8];	//fram_buffer��ÿһ��Ϊ8bit,һ��128*8�� 8bit
	const u16 dma_size = 1024;
#elif defined SSD1351_C128_R128
	u16 Frame_Buffer[128][128];
	const u16 dma_size = 32768;
#elif defined SSD1351_C128_R96
	u16 Frame_Buffer[96][128];
	const u16 dma_size = 24567;
#endif


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
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);			//����
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//CS(PB0)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);						//����

#if DMA_MODE	
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);			//ʹ��DMA����
	DMA_DeInit(DMA1_Channel3);									//����DMA1��ͨ��3(SPI1_TX)
	DMA_InitStructure.DMA_PeripheralBaseAddr = (u32)&SPI1->DR;	//DMA�������ַ -> SPI1
	DMA_InitStructure.DMA_MemoryBaseAddr = (u32)&Frame_Buffer;	//DMA�ڴ����ַ -> FrameBuffer
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;			//���ݴ��䷽��Ϊ����������(SPI)
	DMA_InitStructure.DMA_BufferSize = /*1*/dma_size<<1;				//DMAͨ����DMA����Ĵ�СΪ 128 Byte
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	//�����ַ�Ĵ���������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				//�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;			//���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;				//��������ͨģʽ,��ѭ���ɼ�
	DMA_InitStructure.DMA_Priority = DMA_Priority_High;			//DMAͨ��3�����ȼ�Ϊ �ϸ߼� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;				//DMAͨ��3 �����ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);

	//DMA_Cmd(DMA1_Channel3, ENABLE);//��oleda_refresh()������DMA, ���ж�Handler()�н�ֹDMA
#endif

	SPI1_Init();//��ʼ��SPI1,(������������ Baud = 18M )
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
#ifdef SSD1306_C128_R64
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC����
	oled_write_Byte(0x14,OLED_CMD);  //DCDC ON
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
	oled_write_Byte(0xAF,OLED_CMD);  //Display ON
#endif
}

//�ر�OLED��ʾ     
void oled_display_off(void)
{
#ifdef SSD1306_C128_R64
	oled_write_Byte(0x8D,OLED_CMD);  //SET DCDC����
	oled_write_Byte(0x10,OLED_CMD);  //DCDC OFF
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
	oled_write_Byte(0xAE,OLED_CMD);  //Display OFF
#endif
}

//����buffer����������(����Ļ��ʱûӰ��,�������ˢ��)
//������������Ǿ���, ��x0<x1,y0<y1
//��0�����ǰ���������
#if defined SSD1306_C128_R64
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
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
void oled_buffer_clear(u8 x0, u8 y0, u8 x1, u8 y1)
{
	u8 x, y;
	for(y=y0; y<y1+1; y++){
		for(x=x0; x<x1+1; x++){
			Frame_Buffer[y][x] = 0x0000;	//WHITE:0xFFFF, BLACK:0x0000
			//Frame_Buffer[y][x*2+1] = 0x00;
		}
	}
}
#endif

#if defined SSD1306_C128_R64
//��buffer����������GRAM, ��ˢ��һ֡��Ļͼ��
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
#elif defined SSD1351_C128_R128
void oled_refresh(void)
{
	u8 i,n;
	//����Ĭ��ֵ, ���Բ������Խ�ʡʱ��
//	oled_write_Byte(0x15,OLED_CMD);	//Set Column Address
//	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
//	oled_write_Byte(0x7F,OLED_DATA);//High Adress 127	(reset)
//	oled_write_Byte(0x75,OLED_CMD);	//Set Row Address
//	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
//	oled_write_Byte(0x7F,OLED_DATA);//High Adress 127	(reset is 127)
	#if DMA_MODE
		//DMA_Cmd(DMA1_Channel3, DISABLE);	//�ر�DMA����
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);				
				oled_write_Byte(0x15,OLED_CMD);	//Set Column Address
				oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
				oled_write_Byte(0x7F,OLED_DATA);//High Adress 127	(reset)
				oled_write_Byte(0x75,OLED_CMD);	//Set Row Address
				oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
				oled_write_Byte(0x7F,OLED_DATA);//High Adress 127	(reset is 127)
		oled_write_Byte(0x5C,OLED_CMD);	//Write GRAM Command
	#else
		oled_write_Byte(0x5C,OLED_CMD);	//Write GRAM Command
	#endif
	
	#if DMA_MODE	//��DMA����ˢ��Frame����(��û�д�DMA���ж�)
		DMA_ClearFlag(DMA1_FLAG_TC3);
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		/*!< DMA1 Channel3 enable */
		DMA_Cmd(DMA1_Channel3, ENABLE);
		
		OLED_DC = OLED_DATA;
		OLED_CS = 0;
		for(i=0; i<HEIGHT; i++){		//128 Raw
			for(n=0; n<WIDTH; n++){		//128 Column * 2 Byte color-data
//				SPI1_ReadWriteByte(Frame_Buffer[i][n*2]);
//				SPI1_ReadWriteByte(Frame_Buffer[i][n*2+1]);
				while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
				SPI_I2S_SendData(SPI1, (u8)(Frame_Buffer[i][n]>>8));
				while(SPI_I2S_GetFlagStatus(SPI1, SPI_I2S_FLAG_TXE) == RESET);
				SPI_I2S_SendData(SPI1, (u8)Frame_Buffer[i][n]);
			}
		}
		OLED_CS=1;

	#else
		for(i=0; i<HEIGHT; i++){		//128 Raw
			for(n=0; n<WIDTH; n++){		//128 Column * 2 Byte color-data
				//oled_write_Byte(Frame_Buffer[i][n*2],OLED_DATA);
				//oled_write_Byte(Frame_Buffer[i][n*2+1],OLED_DATA);
				
				oled_write_Byte((u8)(Frame_Buffer[i][n]>>8),OLED_DATA);
				oled_write_Byte((u8)Frame_Buffer[i][n],OLED_DATA);
			}
		}
	#endif
}
#elif defined SSD1351_C128_R96
void oled_refresh(void)
{	
	u8 i,n;
//	oled_write_Byte(0x15,OLED_CMD);	//Set Column Address
//	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
//	oled_write_Byte(0x7F,OLED_DATA);//High Adress 127	(reset)
	oled_write_Byte(0x75,OLED_CMD);	//Set Row Address
	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
	oled_write_Byte(0x5F,OLED_DATA);//High Adress 95	(reset is 127)	
	oled_write_Byte(0x5C,OLED_CMD);	//Write GRAM Command
		
	#if DMA_MODE	//��DMA����ˢ��Frame����(��û�д�DMA���ж�)
		DMA_ClearFlag(DMA1_FLAG_TC3);
		SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, ENABLE);
		/*!< DMA1 Channel3 enable */
		DMA_Cmd(DMA1_Channel3, ENABLE);
		
		OLED_DC = OLED_DATA;
		OLED_CS = 0;
		for(i=0; i<HEIGHT; i++){		//96 Raw
			for(n=0; n<WIDTH; n++){		//128 Column * 2 Byte color-data
				SPI1_ReadWriteByte(Frame_Buffer[i][n*2]);
				SPI1_ReadWriteByte(Frame_Buffer[i][n*2+1]);
			}
		}
		OLED_CS=1;
		
	#else
		for(i=0; i<HEIGHT; i++){		//96 Raw
			for(n=0; n<WIDTH; n++){		//128 Column * 2 Byte color-data
				oled_write_Byte(Frame_Buffer[i][n*2],OLED_DATA);
				oled_write_Byte(Frame_Buffer[i][n*2+1],OLED_DATA);
			}
		}
	#endif
}
#endif

//����
void oled_clear(void)
{
	oled_buffer_clear(0, 0, WIDTH-1, HEIGHT-1);//Ĭ����0ȫ��
	oled_refresh();
}

#if defined SSD1306_C128_R64
//���, x(0~127),y(0~63)�����ص�����,ע��(0,0)���� ���Ͻ�
//mode:1 (��GRAM��)�������ص�    0: (��GRAM��)Ϩ�����ص�
void oled_draw_point(u8 x, u8 y, u16 mode)
{
	u8 page, offset;
	u8 temp=0;
	if(x>=WIDTH || y>=HEIGHT) return;	//������Χ
	page = y/8;					//���ڵ�ҳ
	offset = y%8;				//ҳ��ƫ��
	temp = 0x01 << offset;
	
	//��Ϊ����buffer����, ������Ҫlocate()����ѡ������!
	if(mode)	Frame_Buffer[x][page] |= temp;
	else		Frame_Buffer[x][page] &= ~temp;
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
//���, x(0~127),y(0~127)�����ص�����,ע��(0,0)���� ���Ͻ�
//color: 16bit(565)RGB
void oled_draw_point(u8 x, u8 y, u16 color)
{
	if(x>=WIDTH || y>=HEIGHT) return;	//������Χ
	
	//��Ϊ����buffer����, ������Ҫlocate()����ѡ������!
	Frame_Buffer[y][x] = color;
	//Frame_Buffer[y][2*x+1]= (u8)(color);
}
#endif

#if defined SSD1306_C128_R64
//������������ʾ�ַ�,lib֧��0806,1206,1608
//x(0~127),y(0~63)     /mode:0,������ʾ(��������)   1,������ʾ
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u16 mode)
{
	u8 temp,i,k;
	u8 y0=y;			//��¼ԭʼy
//	u8 x0=x;			//��¼ԭʼx
	chr = chr - ' ';	//�õ�ƫ�ƺ��ֵ(���ڵ�����)
	u8 csize=0;			//һ���ַ�ռ���ֽ���,1206Ϊ12,1608Ϊ16
	switch(lib){
		case  8: csize=6;	break;
		case 12: csize=12;	break;
		case 16: csize=16;	break;
		default: return;				//û�ж�Ӧ���ַ���
	}
	for(i=0; i<csize; i++)
	{
		if((x>=WIDTH)||(y>=HEIGHT))		//����ʾ��ûԽ��Ĳ���(�������ݶ��ź;��)����Ҳ�Խ��, ������
			continue;
		switch(lib){
			case  8: temp = ascii_0806[chr][i];	break;
			case 12: temp = ascii_1206[chr][i];	break;
			case 16: temp = ascii_1608[chr][i];	break;
			default: break;
		}
		for(k=0; k<8; k++)
		{
			if(temp&0x01)	oled_draw_point(x,y,mode);//1����ǰΪ�ַ������ϵ�ĳһ��
			else 			oled_draw_point(x,y,!mode);
			temp >>= 1;
			y++;							//׼�������·���һ��Ԫ�ص�
			if((y-y0)==lib){				//���1206�赽��12����,�򼴿̻�����һ��
				y = y0;
				x++;
				break;						//���1206�����,������һ�м������
			}
		}
	}
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
//������������ʾ�ַ�,lib֧��0806,1206,1608
//x(0~127),y(0~63)     /mode:0,������ʾ(��������)   1,������ʾ
void oled_show_char(u8 x, u8 y, u8 chr, u8 lib, u16 front_color, u16 back_color)
{      			    
	u8 temp,i,k;
	u8 y0=y;			//��¼ԭʼy
//	u8 x0=x;			//��¼ԭʼx
	chr = chr - ' ';		//�õ�ƫ�ƺ��ֵ(���ڵ�����)
	u8 csize=0;
	switch(lib){
		case  8: csize=6;	break;
		case 12: csize=12;	break;
		case 16: csize=16;	break;
		default: return;	//û�ж�Ӧ���ַ���
	}
	for(i=0; i<csize; i++)
	{
		if((x>=WIDTH)||(y>=HEIGHT))		//����ʾ��ûԽ��Ĳ���(�������ݶ��ź;��)����Ҳ�Խ��, ������
			continue;
		switch(lib){
			case  8: temp = ascii_0806[chr][i];	break;
			case 12: temp = ascii_1206[chr][i];	break;
			case 16: temp = ascii_1608[chr][i];	break;
			default: break;
		}
		for(k=0; k<8; k++)
		{
			if(temp&0x01)	oled_draw_point(x,y,front_color);//1����ǰΪ�ַ������ϵ�ĳһ��
			else 			oled_draw_point(x,y,back_color);
			temp >>= 1;
			y++;							//׼�������·���һ��Ԫ�ص�
			if((y-y0)==lib){				//���1206�赽��12����,�򼴿̻�����һ��
				y = y0;
				x++;
				break;						//���1206�����,������һ�м������
			}
		}
	}
}
#endif

//m^n���ݺ���,���ļ����ڲ�����
u32 mypow(u8 m,u8 n)
{
	u32 result=1;	 
	while(n--)
		result *= m;    
	return result;
}

#if defined SSD1306_C128_R64
//��ʾһ������(��λ��0����ʾ)
//x,y:����	  /len:��λ��   /lib:����   /mode:0,������ʾ  1,������ʾ			  
void oled_show_num_hide0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 mode)
{         	
	u8 temp;
	u8 enshow=0;
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;			//�������ҵõ�ÿһλ����
		
		if(temp==0 && t==len-1){					//������һ������0,����ʾ'0'
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==0){	//���ǰ��λ�г���0,��Ŀǰ��δ���ַ�����,����ʾ' '
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,' ',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==1){	//���ǰ��λ�г���0,���������ֹ�������,����ʾ'0'
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else{										//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
			else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		}		
	}
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128			  
void oled_show_num_hide0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 front_color, u16 back_color)
{         	
	u8 temp;
	u8 enshow=0;
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;			//�������ҵõ�ÿһλ����
		
		if(temp==0 && t==len-1){					//������һ������0,����ʾ'0'
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,front_color,back_color);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,front_color,back_color);
		}
		else if(temp==0 && t<len-1 && enshow==0){	//���ǰ��λ�г���0,��Ŀǰ��δ���ַ�����,����ʾ' '
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,front_color,back_color);
			else		oled_show_char(x + t*(lib/2),y,' ',lib,front_color,back_color);
		}
		else if(temp==0 && t<len-1 && enshow==1){	//���ǰ��λ�г���0,���������ֹ�������,����ʾ'0'
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,front_color,back_color);
			else		oled_show_char(x + t*(lib/2),y,'0',lib,front_color,back_color);
		}
		else{										//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,front_color,back_color);
			else		oled_show_char(x+t*(lib/2),y,temp+'0',lib,front_color,back_color);
		}
	}
}
#endif

#if defined SSD1306_C128_R64
//��ʾһ������(��λ��0��Ȼ��ʾ)    /mode:0,������ʾ  1,������ʾ
void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 mode)
{         	
	u8 temp;						   
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//�������ҵõ�ÿһλ����
		if(lib==8)	oled_show_char(x + t*6,y,temp+'0',lib,mode);//0806��Ϊ2+8/2=6
		else		oled_show_char(x + t*(lib/2),y,temp+'0',lib,mode);//1206,1608���Ǹߵ�1/2
	}
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
void oled_show_num_every0(u8 x, u8 y, u32 num, u8 len, u8 lib, u16 front_color, u16 back_color)
{         	
	u8 temp;						   
	for(u8 t=0;t<len;t++)
	{
		temp=(num/mypow(10,len-t-1))%10;	//�������ҵõ�ÿһλ����
		if(lib==8)	oled_show_char(x + t*6,y,temp+'0',lib,front_color,back_color);//0806��Ϊ2+8/2=6
		else		oled_show_char(x + t*(lib/2),y,temp+'0',lib,front_color,back_color);//1206,1608���Ǹߵ�1/2
	}
}
#endif

/**
  * @brief  ��ʾС��(�����ɸ�)
  * @note	ע�⸺�Ż�ռ�õ�1��Ŀռ�, Ĭ�ϲ���ʾ����
  * @param  fnum: ��Ҫ��ʾ��С��(float)
  *			len1: ���������ֵĳ���, ע�⸺������Ϊ���Ŷ�������ʾ1��
  *			len2: ��С�����ֵĳ���, ����������λС��
  *			mode: 0,������ʾ  1,������ʾ
  * @retval None
  */
#if defined SSD1306_C128_R64
void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u16 mode)
{
	u32 integer = 0;	//���յ�С���������������
	u16 pwr = 1;		//�������ʵ��С���Ĵ��ݱ���(1,10,100,1000, ...)
	u8 len22 = len2;	//��Ϊlen2���·����Լ�, ����Ҫ���Ᵽ��len2�ĳ�ֵ
	while(len2){
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
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
void oled_show_float(u8 x, u8 y, float fnum, u8 len1, u8 len2, u8 lib, u16 front_color, u16 back_color)
{
	u32 integer = 0;	//���յ�С���������������
	u16 pwr = 1;		//�������ʵ��С���Ĵ��ݱ���(1,10,100,1000, ...)
	u8 len22 = len2;	//��Ϊlen2���·����Լ�, ����Ҫ���Ᵽ��len2�ĳ�ֵ
	while(len2){
		pwr *= 10;
		len2--;
	}
	if(fnum < 0)//����
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, '-', lib, front_color, back_color);//�������������1����ʾ����
		fnum = fabs(fnum);
		integer = fnum;	//����������ֵ�ֵ
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, front_color, back_color);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, front_color, back_color);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//���С�����ֵ�ֵ
		oled_show_num_every0(x, y, integer, len22, lib, front_color, back_color);
	}
	else		//����
	{
		if(lib == 8)	x -= 6;
		else			x -= (lib/2);
		oled_show_char(x, y, ' ', lib, front_color, back_color);//�������������1����ʾ�ո񷽱㸲�ǵ�����
		integer = fnum;	//����������ֵ�ֵ
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		oled_show_num_hide0(x, y, integer, len1, lib, front_color, back_color);
		if(lib == 8)	x += len1*6;
		else			x += len1*(lib/2);
		oled_show_char(x, y, '.', lib, front_color, back_color);
		if(lib == 8)	x += 6;
		else			x += (lib/2);
		integer = (u32)(fnum * pwr) % pwr;	//���С�����ֵ�ֵ
		oled_show_num_every0(x, y, integer, len22, lib, front_color, back_color);
	}
}
#endif

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
#if defined SSD1306_C128_R64
void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u16 mode)
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
			if((x>=128)||(y>=64))
				continue;
			temp = matrix[i];
			for(k=0; k<8; k++)
			{
				if(temp & 0x80)		oled_draw_point(x, y, mode);
				else				oled_draw_point(x, y, !mode);
				temp <<= 1;
				y++;
				if((y - y0)==lib){
					y = y0;
					x++;
					break;	//�������һ����������һ�еĻ���
				}
			}
		}
	}
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
void oled_show_GBK(u8 x, u8 y, u8 gbk_H, u8 gbk_L, u8 lib, u16 front_color, u16 back_color)
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
			if((x>=WIDTH)||(y>=HEIGHT))
				continue;
			temp = matrix[i];
			for(k=0; k<8; k++)
			{
				if(temp & 0x80)		oled_draw_point(x, y, front_color);
				else				oled_draw_point(x, y, back_color);
				temp <<= 1;
				y++;
				if((y - y0)==lib){
					y = y0;
					x++;
					break;	//�������һ����������һ�еĻ���
				}
			}
		}
	}
}
#endif

#if defined SSD1306_C128_R64
//��ʾ�ַ���
//x,y:�������    /lib:�����С    /mode:0,������ʾ  1,������ʾ
void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u16 mode)
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
			if(x>128-lib/2)						//���ʣ���������һ�뺺�ֶ�������ʾ����
			{
				x = x0;
				y = y + lib + 1;
			}
			else if(x>128-lib+1)
			{
				//���ֻʣ������ֵĿռ�, ֻ������ʾ���������ĸ�����, ���඼Ӧ�ü��̻���
				if(((p[i]!=0xA1)&&(p[i+1]!=0xA3))		/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i]!=0xAC))		/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i+1]!=0xBA))	/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i+1]!=0xBB))	/* ��*/)
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
			//continue;
		}
		else
		{
			oled_show_char(x, y, p[i], lib, mode);
			i++;
			if(lib==8) x += 6;
			else x += lib/2;
			
			/* ���ѳ�����˵������string�����Ҳ�߽�, ���м�����ʾ 0806/1206:x+2, 1608:x+0, y+1 */
			/* ��string���Ҳ����߽粻�㹻��ʾ��һ���ַ�, ������ʾ */
			/* ����Ҫע����� ��� �� ���� ��ʵ����ģ�������, �Ͼ����ǲ�̫���ʻ�����ʾ */
			if(((lib==8)&&(x>=126)) || ((x>=128-lib/4+1)&&(lib!=8)))//����ַ���ȶ�����
			{
				x = x0;
				if(lib==16)	y = y + lib;
				else		y = y + lib + 1;
			}
			else if(((lib==8)&&(x>123)&&(p[i]!='.')&&(p[i]!=',')) \
				|| ((lib!=8)&&(x>128-lib/2+1)&&(p[i]!='.')&&(p[i]!=',')))
			{
				x = x0;
//				if(lib==16)	x = x0;
//				else		x = x0 + 2;
				if(lib>=16)	y = y + lib;
				else		y = y + lib + 1;
			}
			else;
		}
	}
}
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
void oled_show_string(u8 x, u8 y, const u8 *p, u8 lib, u16 front_color, u16 back_color)
{	
	u8 i=0;
	u8 x0=x;
	while(p[i] != '\0')
	{		
		if(p[i]=='\r'){							//�������'\r'��	
//			oled_show_char(x,y,' ',lib,mode);	//������� ���� ��ʾһ��' '
			i++;
			continue;							//�����·������ٽ���whileѭ����ֹ����ƫ����ɼ��һ���ַ����
		}
		else if(p[i]=='\n'){					//���������'\n'��
//			oled_show_char(x,y,'\\',lib,mode);	//��ʾһ��slash
			//oled_show_char(x,y,'n',lib,mode);	//��ʾһ��'n'
			x = (lib==16)?0:1;					//������ʾ x+1 or x+0
			y = y+lib+1;						//������ʾ y+1
			i++;
			continue;							//�����·������ٽ���whileѭ��
		}
		else if(p[i]>=0x81)						//���������
		{
			if(x>128-lib/2){					//���ʣ���������һ�뺺�ֶ�������ʾ����
				x = x0;
				y = y + lib + 1;
			}
			else if(x>128-lib+1){
				//ֻʣ�¸պò���һ�����ֵĿռ�, ֻ������ʾ���������ĸ�����, ���඼Ӧ�ü��̻���
				if(((p[i]!=0xA1)&&(p[i+1]!=0xA3))		/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i]!=0xAC))		/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i+1]!=0xBA))	/* ��*/ \
					|| ((p[i]!=0xA3)&&(p[i+1]!=0xBB)))	/* ��*/
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
				oled_show_GBK(x, y, p[i], p[i+1], lib, front_color, back_color);
				i += 2;
				x += lib;
			}
			//continue;
		}
		else
		{
			oled_show_char(x, y, p[i], lib, front_color, back_color);
			i++;
			if(lib==8)	x += 6;
			else 		x += lib/2;
			
			/*����1608������, ����һ�ɲ���ƫ�ơ�*/
			/* ����ַ�������������ʱ�պ������ո������ȥ, ��Ӧ�ðѿո����ʾ����һ�п�ͷ */
			if((p[i]==' ')&&(x>=128-lib))
			{
				;//��ָ��ᵼ��ǿ���ڵ�ǰ����ʾ�ո�, �ȴ���һѭ��ʱ�Զ�����(���������ڷ�ɫʱ��Ȼ����ֿո��, �ʲ�Ӧ�����ո������ʾ)
			}
			/* ��string���Ҳ����߽粻�㹻��ʾ��һ���ַ�, ������ʾ */
			/* ����Ҫע����� ��� �� ���� ��ʵ����ģ�������, �Ͼ����ǲ�̫���ʻ�����ʾ */
			else if(((lib==8)&&(x>=126)) || ((x>=128-lib/4+1)&&(lib!=8)))//����ַ���ȶ�����
			{
				x = x0;
				if(lib==16)	y = y + lib;
				else		y = y + lib + 1;
			}
			//ֻʣ�¸պò���һ���ַ����
			else if(((lib==8)&&(x>123)&&(p[i]!='.')&&(p[i]!=',')) \
				|| ((x>128-lib/2+1)&&(lib!=8)&&(p[i]!='.')&&(p[i]!=',')))
			{
				x = x0;
				if(lib==16)	y = y + lib;
				else		y = y + lib + 1;
			}
			else;
		}
	}
}
#endif

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
#if defined SSD1306_C128_R64
void oled_show_Bin_Array(u8 x, u8 y, const u8 *pPic, u16 mode)//3��badapple��bin��ʽ: 085*64, 106*80, 170*128
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
#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
//��˵��ɫ��ͼ������Ҫdraw_point, ����draw_line����Ҫ���
void oled_show_Bin_Array(u8 x, u8 y, const u8 *pPic, u8 width, u8 height, u8 depth, u8 mono_byte_turn)
{         	
	u8 i, j;
	u16 n=0;
	if(depth == 0x01)						//����� ��ɫͼƬ
	{
		//���ͼƬ�߶ȴ��ڱ���0�����, Ӧ����/8����һ�����µ����ص�
		//u8 turn = height%8 ? (height/8+1):(height/8);
		u8 temp;
		u8 t;
		for(i=0; i<width; i++){
			for(j=0; j<mono_byte_turn; j++){//mono_byte_turn��ʾ��ɫ��һ�а��������ֽ�
				temp = 0x80;
				for(t=0; t<8; t++){
					
					if(i+x >= WIDTH)		break;
					if(j*8+t+y >= HEIGHT)	break;
					
					if(pPic[n] & temp){		//��ȡbin�õ���0xFF�����ɫ
						Frame_Buffer[j*8+t+y][i+x] = BLACK;
						//Frame_Buffer[j*8+t+y][(i+x)*2+1] = (u8)BLACK;
					}
					else{
						Frame_Buffer[j*8+t+y][i+x] = CYAN;
						//Frame_Buffer[j*8+t+y][(i+x)*2+1] = (u8)CYAN;
					}
//					if(i+x+1 >= WIDTH)
//						return;
//					if(j*8+t+y+1 >= HEIGHT)
//						break;
					if(j*8+t+1 == height)	//�����ʾ��ͼƬ�߶���, ��0������������ʾ
						break;
					
					temp >>= 1;
				}
				n += 1;
			}
		}
	}
	else if(depth == 16)					//����� 16λ�߲�ɫͼƬ(��ע�⡿SD��ͼƬ������ô˺�������ʾ,�����ⲿֱ�Ӳٿ�Frame[])
	{
		for(i=0; i<width; i++){
			for(j=0; j<height; j++){
				Frame_Buffer[j+y][i+x] = ((u16)pPic[n]<<8)+pPic[n+1];
				//Frame_Buffer[j+y][i+x] = pPic[n+1];
				if(i+x+1 >= WIDTH)
					return;
				if(j+y+1 >= HEIGHT)
					break;
				
				n+=2;
			}
		}
	}
	else if(depth == 24)					//����� 4λ���ɫͼƬ
	{
	}
	else ;
}
#endif
//void oled_show_ColorImage(u8 x, u8 y, const u8 *pPic, u16 mode)
//{
//	u8 i, j;
//	u16 n=0;
//	for(i=0;i<HEIGHT;i++){
//		for(j=0; j<WIDTH; j++){
//			Frame_Buffer[i][j*2] = RGB_lady[n];		//��˵��ͼ������Ҫdraw_point
//			Frame_Buffer[i][j*2+1] = RGB_lady[n+1];	//����draw_line����Ҫ���
//			n+=2;
//		}
//	}
//}
void oled_show_ColorImage(u8 x, u8 y, const u8 *pPic, u8 width, u8 height)
{
	u8 i, j;
	u16 n=0;
	for(i=0; i<width; i++){
		for(j=0; j<height; j++){
			Frame_Buffer[j+y][(i+x)*2] = pPic[n];		//��˵��ͼ������Ҫdraw_point
			Frame_Buffer[j+y][(i+x)*2+1] = pPic[n+1];	//����draw_line����Ҫ���
			n+=2;
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
void oled_show_Icon(u8 x, u8 y, const u8 Icon, u8 height, u16 mode)
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


#if defined SSD1306_C128_R64
//��ʼ��SSD1306					    
void oled_ssd1306_init(void)//Ĭ�ϲ����� (Page_0,Column_0)��oled���Ͻǵ�λ��
{
	OLED_RST = 1;
	delay_ms(30);
	OLED_RST = 0;//�͵�ƽ��Ч
	delay_ms(125);
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

#elif defined SSD1351_C128_R96 || defined SSD1351_C128_R128
//��ʼ��SSD1351					    
void oled_ssd1351_init(void)//Ĭ�ϲ����� (Page_0,Column_0)��oled���Ͻǵ�λ��
{
	OLED_RST = 1;
	delay_ms(30);
	OLED_RST = 0;//�͵�ƽ��Ч
	delay_ms(145);
	OLED_RST = 1;
	delay_ms(5);
	
	oled_write_Byte(0xFD,OLED_CMD);	//Lock/Unlock Command
	oled_write_Byte(0x12,OLED_DATA);//Unclock SSD1351 for receive command
	oled_write_Byte(0xFD,OLED_CMD);	//Lock/Unlock Command
	oled_write_Byte(0xB1,OLED_DATA);//Unclock cmd(s) A2,B1,B3,BB,BE
	oled_write_Byte(0xAE,OLED_CMD);	//display OFF
	
	oled_write_Byte(0xB3,OLED_CMD);	//Clock Divider Command
	oled_write_Byte(0xF1,OLED_DATA);//Freq_osc=0xF(0xF is max), divider D=0x1(min)
	oled_write_Byte(0xCA,OLED_CMD);	//MUX Ratio Command
	oled_write_Byte(HEIGHT-1,OLED_DATA);//Set 128 or 96 MUX(reset is 0x7F i.e.128 MUX)
	oled_write_Byte(0xA2,OLED_CMD);	//Display Offset Command
	oled_write_Byte(0x00,OLED_DATA);//Set Offset 0
	oled_write_Byte(0xA1,OLED_CMD);	//Display Start Line Command
	oled_write_Byte(0x00,OLED_DATA);//Set Start at line 0
	oled_write_Byte(0xA0,OLED_CMD);	//Re-Map & Color Depth & Scan Mode Command
	oled_write_Byte(0x74,OLED_DATA);//0x04: Re-map(0->127), Scan=Horizontal, Color=R-G-B(C-B-A)
									//0x70: Scan(MUX->1), Enable Split, Color Depth is 65k(565)
	oled_write_Byte(0xB5,OLED_CMD);	//GPIO Command
	oled_write_Byte(0x00,OLED_DATA);//Soft:Disable(after all is Floating in FPC)
	oled_write_Byte(0xAB,OLED_CMD);	//Function Select Command
	oled_write_Byte(0x01,OLED_DATA);//Enable Internal VDD Regulator
	
	oled_write_Byte(0xB4,OLED_CMD);	//Segment Low Voltage Command(VSL pin)
	oled_write_Byte(0xA0,OLED_DATA);//Use External VSL(need connect with resistor and diode to GND)
	oled_write_Byte(0xB5,OLED_DATA);//2nd data of cmd.B4
	oled_write_Byte(0x55,OLED_DATA);//3rd data of cmd.B4
	
	oled_write_Byte(0xC1,OLED_CMD);	//Contrast Current for Color A,B,C Command
	oled_write_Byte(0xC8,OLED_DATA);//Contrast Value for b(recommend 0xC8)
	oled_write_Byte(0xB0,OLED_DATA);//Contrast Value for g(recommend 0x80)
	oled_write_Byte(0xC2,OLED_DATA);//Contrast Value for r(recommend 0xC8)
	
	oled_write_Byte(0xC7,OLED_CMD);	//Master Contrast Current Control Command
	oled_write_Byte(0x0F,OLED_DATA);//Real output current for RGB is as before(range 1/16 -> 16/16)
	
	oled_write_Byte(0xB8,OLED_CMD);	//Gamma Look-up Table for Gray Scale Command
	oled_write_Byte(0x02,OLED_DATA);//Gray Scale Level  1
	oled_write_Byte(0x03,OLED_DATA);//Gray Scale Level  2
	oled_write_Byte(0x04,OLED_DATA);//Gray Scale Level  3
	oled_write_Byte(0x05,OLED_DATA);//Gray Scale Level  4
	oled_write_Byte(0x06,OLED_DATA);//Gray Scale Level  5
	oled_write_Byte(0x07,OLED_DATA);//Gray Scale Level  6
	oled_write_Byte(0x08,OLED_DATA);//Gray Scale Level  7
	oled_write_Byte(0x09,OLED_DATA);//Gray Scale Level  8
	oled_write_Byte(0x0A,OLED_DATA);//Gray Scale Level  9
	oled_write_Byte(0x0B,OLED_DATA);//Gray Scale Level 10
	oled_write_Byte(0x0C,OLED_DATA);//Gray Scale Level 11
	oled_write_Byte(0x0D,OLED_DATA);//Gray Scale Level 12
	oled_write_Byte(0x0E,OLED_DATA);//Gray Scale Level 13
	oled_write_Byte(0x0F,OLED_DATA);//Gray Scale Level 14
	oled_write_Byte(0x10,OLED_DATA);//Gray Scale Level 15 -> setting 16
	oled_write_Byte(0x11,OLED_DATA);//Gray Scale Level 16
	oled_write_Byte(0x12,OLED_DATA);//Gray Scale Level 17
	oled_write_Byte(0x13,OLED_DATA);//Gray Scale Level 18 -> setting 19
	oled_write_Byte(0x15,OLED_DATA);//Gray Scale Level 19 -> setting 21
	oled_write_Byte(0x17,OLED_DATA);//Gray Scale Level 20 -> setting 23
	oled_write_Byte(0x19,OLED_DATA);//Gray Scale Level 21
	oled_write_Byte(0x1B,OLED_DATA);//Gray Scale Level 22
	oled_write_Byte(0x1D,OLED_DATA);//Gray Scale Level 23
	oled_write_Byte(0x1F,OLED_DATA);//Gray Scale Level 24 -> setting 31
	oled_write_Byte(0x21,OLED_DATA);//Gray Scale Level 25
	oled_write_Byte(0x23,OLED_DATA);//Gray Scale Level 26
	oled_write_Byte(0x25,OLED_DATA);//Gray Scale Level 27
	oled_write_Byte(0x27,OLED_DATA);//Gray Scale Level 28
	oled_write_Byte(0x2A,OLED_DATA);//Gray Scale Level 29
	oled_write_Byte(0x2D,OLED_DATA);//Gray Scale Level 30
	oled_write_Byte(0x30,OLED_DATA);//Gray Scale Level 31
	oled_write_Byte(0x33,OLED_DATA);//Gray Scale Level 32 -> setting 51
	oled_write_Byte(0x36,OLED_DATA);//Gray Scale Level 33
	oled_write_Byte(0x39,OLED_DATA);//Gray Scale Level 34
	oled_write_Byte(0x3C,OLED_DATA);//Gray Scale Level 35
	oled_write_Byte(0x3F,OLED_DATA);//Gray Scale Level 36
	oled_write_Byte(0x42,OLED_DATA);//Gray Scale Level 37
	oled_write_Byte(0x45,OLED_DATA);//Gray Scale Level 38
	oled_write_Byte(0x48,OLED_DATA);//Gray Scale Level 39
	oled_write_Byte(0x4C,OLED_DATA);//Gray Scale Level 40
	oled_write_Byte(0x50,OLED_DATA);//Gray Scale Level 41 -> setting 80
	oled_write_Byte(0x54,OLED_DATA);//Gray Scale Level 42
	oled_write_Byte(0x58,OLED_DATA);//Gray Scale Level 43
	oled_write_Byte(0x5C,OLED_DATA);//Gray Scale Level 44
	oled_write_Byte(0x60,OLED_DATA);//Gray Scale Level 45
	oled_write_Byte(0x64,OLED_DATA);//Gray Scale Level 46
	oled_write_Byte(0x68,OLED_DATA);//Gray Scale Level 47
	oled_write_Byte(0x6C,OLED_DATA);//Gray Scale Level 48
	oled_write_Byte(0x70,OLED_DATA);//Gray Scale Level 49
	oled_write_Byte(0x74,OLED_DATA);//Gray Scale Level 50 -> setting 116
	oled_write_Byte(0x78,OLED_DATA);//Gray Scale Level 51
	oled_write_Byte(0x7D,OLED_DATA);//Gray Scale Level 52
	oled_write_Byte(0x82,OLED_DATA);//Gray Scale Level 53
	oled_write_Byte(0x87,OLED_DATA);//Gray Scale Level 54
	oled_write_Byte(0x8C,OLED_DATA);//Gray Scale Level 55
	oled_write_Byte(0x91,OLED_DATA);//Gray Scale Level 56
	oled_write_Byte(0x96,OLED_DATA);//Gray Scale Level 57
	oled_write_Byte(0x9B,OLED_DATA);//Gray Scale Level 58
	oled_write_Byte(0xA0,OLED_DATA);//Gray Scale Level 59 -> setting 160
	oled_write_Byte(0xA5,OLED_DATA);//Gray Scale Level 60
	oled_write_Byte(0xAA,OLED_DATA);//Gray Scale Level 61
	oled_write_Byte(0xAF,OLED_DATA);//Gray Scale Level 62
	oled_write_Byte(0xB4,OLED_DATA);//Gray Scale Level 63 -> setting 180
	
	oled_write_Byte(0xB1,OLED_CMD);	//Phase_1, Phase_2 Period Command
	oled_write_Byte(0x32,OLED_DATA);//Phase1=5DCLK(5-31), Phase2=3DCLK(3-15)
	
	oled_write_Byte(0xBB,OLED_CMD);	//Set Pre-charge voltage
	oled_write_Byte(0x17,OLED_DATA);//range 00h-1Fh(reset is 0x17h)
	
	//�ֲ����Ҳ���B2hָ��
	oled_write_Byte(0xB2,OLED_CMD);	//Enhance Driving Scheme Capability
	oled_write_Byte(0xA4,OLED_DATA);
	oled_write_Byte(0x00,OLED_DATA);
	oled_write_Byte(0x00,OLED_DATA);
	
	oled_write_Byte(0xB6,OLED_CMD);	//Second Pre-Charge Period(i.e.Phase_3) Command
	oled_write_Byte(0x01,OLED_DATA);//Phase3=1DCLK
	oled_write_Byte(0xBE,OLED_CMD);	//COMH Voltage
	oled_write_Byte(0x05,OLED_DATA);//0.82*VCC(reset is 0x05)
	
	oled_write_Byte(0xA6,OLED_CMD);//Reset to Noramal Display(reset)
	oled_buffer_clear(0, 0, WIDTH-1, HEIGHT-1);
	oled_write_Byte(0xAF,OLED_CMD);//Display ON
	
	
	
	
	//����7�п���ɾ��
//	SPI_I2S_DMACmd(SPI1, SPI_I2S_DMAReq_Tx, DISABLE);
//	oled_write_Byte(0x15,OLED_CMD);	//Set Column Address
//	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
//	oled_write_Byte(WIDTH-1,OLED_DATA);//High Adress 127	(reset)
//	oled_write_Byte(0x75,OLED_CMD);	//Set Row Address
//	oled_write_Byte(0x00,OLED_DATA);//Low Adress 0		(reset)
//	oled_write_Byte(HEIGHT-1,OLED_DATA);//High Adress 127	(reset is 127)
	oled_refresh();
}
#endif



//Plexer�����Ѿ���֤���������д������ȷ��.
