#include "delay.h"
#include "oledfont.h"
#include "oled.h"
#include "spi.h"
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


//PA.234�ֱ���Flash,SDcard,oled��CS��, PC.4��NRF��CS��.
//�ݶ�OLED_CS(PC5),	OLED_RST(PB0), ע��OLED_DC(PC4)Ҳͬʱ����led1, ��spi1���������߻���SPI1_Init()�еõ���ʼ��
void oled_spi_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	//SPI_InitTypeDef SPI_InitStructure;
	/*
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA|RCC_APB2Periph_GPIOC|RCC_APB2Periph_GPIOD, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_12;	//PA.234, PA3������ADC1.CH3ģ������ģʽ
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//�����������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_SetBits(GPIOA, GPIO_Pin_2|GPIO_Pin_4|GPIO_Pin_12);	//��������CS��(PA��),�ر�Flash,oled��spi
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;				//PC.4
 	//GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  	//�����������
	//GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4);						//����NRF_CS��(PC4),�ر�NRF24L01��spi
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2;				//PD.2 --> OLED_DC, led1
	GPIO_Init(GPIOD, &GPIO_InitStructure);
	GPIO_SetBits(GPIOD,GPIO_Pin_2);							//����, ��OLED_DCΪ1, led1Ϩ��	
	*/

	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC, ENABLE);
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;	//DC(PC4), CS(PC5)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOC, &GPIO_InitStructure);
	GPIO_SetBits(GPIOC, GPIO_Pin_4 | GPIO_Pin_5);			//����
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;				//RES(PB0)
 	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  		//����
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;		//Ӱ�칦��
 	GPIO_Init(GPIOB, &GPIO_InitStructure);
	GPIO_SetBits(GPIOB, GPIO_Pin_0);						//����

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
void buffer_clear(void)
{
	u8 y,x;		    
	for(y=0;y<8;y++)
		for(x=0;x<128;x++)
			Frame_Buffer[x][y] = 0x00;
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
		for(n=0;n<128;n++)
			oled_write_Byte(Frame_Buffer[n][i],OLED_DATA);
	}   
}

//����
void oled_clear(void)
{
	buffer_clear();
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
	temp = 0x01<<(offset);
	
	//��Ϊ����buffer����, ������Ҫlocate()����ѡ������!
	if(mode)	Frame_Buffer[x][page] |= temp;
	else			Frame_Buffer[x][page] &= ~temp;
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
		for(i=0;i<6;i++)
		{
			temp = ascii_0806[chr][i];
			for(k=0;k<8;k++)				//ÿ����8����
			{
				if(temp&0x01) oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
				else oled_draw_point(x,y,!mode);
				temp >>= 1; y++;			//׼�������·���һ��Ԫ�ص�
			}
			y = y0; x++;					//�������Ҳ�һ�еĵ�
		}
	}
	else if(lib==12 || lib==16)	//1206,1608
	{
		u8 csize=( lib/8+((lib%8)?1:0) ) * (lib/2);	//һ���ַ�ռ���ֽ���,1206Ϊ12,1608Ϊ16
		for(i=0;i < csize;i++)
		{
			if(lib==12) temp = ascii_1206[chr][i];
			else				temp = ascii_1608[chr][i];
			if(i < csize/2)		//1206�״� ��8��; 1608ÿ����8��
			{
				for(k=0;k<8;k++)
				{
					if(temp&0x01) oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
					else 	oled_draw_point(x,y,!mode);
					temp >>= 1; y++;				//׼�������·���һ��Ԫ�ص�
				}
				y = y0; x++;						//�������Ҳ�һ�еĵ�
			}
			else				//1206�ڶ��� ��4��; 1608ÿ����8��
			{
				y = y0 + 8;
				x = x0 + i - csize/2;	//��λ����ʼ�ڶ���ɨ���������
				for(k=0;k<8;k++)
				{
					if(temp&0x01) oled_draw_point(x,y,mode);//�ж�temp��ĩλ�Ƿ�Ϊ1
					else 	oled_draw_point(x,y,!mode);
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
	else return;//�޶�Ӧ���ַ���
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
			else	oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==0)	//���ǰ��λ�г���0,��Ŀǰ��δ���ַ�����,����ʾ' '
		{
			if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);
			else	oled_show_char(x + t*(lib/2),y,' ',lib,mode);
		}
		else if(temp==0 && t<len-1 && enshow==1)	//���ǰ��λ�г���0,���������ֹ�������,����ʾ'0'
		{
			if(lib==8)	oled_show_char(x + t*6,y,'0',lib,mode);
			else	oled_show_char(x + t*(lib/2),y,'0',lib,mode);
		}
		else								//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
		{
			enshow = 1;
			if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
			else				oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
		}
//		
		/*		
		if(enshow==0 && t<(len-1))			//(len-1)�Ǳ�֤ȫ��ʱ��ʾ��ĩ��0
		{
			if(temp==0)
			{
				if(lib==8)	oled_show_char(x + t*6,y,' ',lib,mode);//0806��Ϊ2+8/2=6
				else	oled_show_char(x + t*(lib/2),y,' ',lib,mode);//1206,1608���Ǹߵ�1/2
				continue;//continue��������һ��ѭ��
			}
			else enshow=1;	 	 
		}
		else ;
		
	 	if(lib==8)	oled_show_char(x + t*6, y, temp + '0',lib,mode);
		else				oled_show_char(x+t*(lib/2),y,temp+'0',lib,mode);
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
		else	oled_show_char(x + t*(lib/2),y,temp+'0',lib,mode);//1206,1608���Ǹߵ�1/2
	}
}

//��ʾ�ַ���(�����Զ�����,�����ʹ��)
//x,y:�������    /lib:�����С    /mode:0,������ʾ  1,������ʾ
void oled_show_string(u8 x, u8 y, const char *p, u8 lib, u8 mode)
{	
	u8 i=0;
	while(p[i] != '\0')
	{		
		if(p[i]=='\r')							//�������'\r'��
		{	
			oled_show_char(x,y,' ',lib,mode);	//������� ���� ��ʾһ��' '
			i++;
			continue;							//�����·������ٽ���whileѭ����ֹ����ƫ����ɼ��һ���ַ����
		}
		else if(p[i]=='\n')						//���������'\n'��
		{
			oled_show_char(x,y,'\\',lib,mode);	//��ʾһ��slash
			//oled_show_char(x,y,'n',lib,mode);	//��ʾһ��'n'
			x=2; y=y+lib+1;						//������ʾ+2,+1
			i++;
			continue;							//�����·������ٽ���whileѭ��
		}
		else
			oled_show_char(x,y,p[i],lib,mode);
		i++;
		if(lib==8) x += 6;
		else x += lib/2;
		
		/* ��string�����Ҳ�߽�, ���м�����ʾ+2,+1 */
		if(x>=128){x=2; y=y+lib+1;}
		else;
	}
}

/**
  * @brief  ��ʾ������ͼƬ(*.bin), �˺���Ŀǰû��������, ��ʱ������ĳ����������ʾ������ݱȵ�binͼƬ
  * @note	ͼƬ��С85*64, �������´�ֱɨ��, ��λ��ǰ, ע���ֿ�ȫ0Ϊ�ո�, ͼƬȫ0Ϊ��ɫͼƬ
  *			ͼƬȫ��ʱwinhex��ʾΪȫ��0XFF, ����oled��ɫ������������
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
	for(u16 i=0;i<680;i++)//��ѡ 680, 1060, 2720
	{
		temp = pPic[i];
		for(u8 j=0;j<8;j++)
		{
			if(temp & 0x80)
				oled_draw_point(x,y,!mode);	//��λ��1�������ص�ԭΪ��ɫ-> Ϩ�����ص�-> mode=0
			else
				oled_draw_point(x,y,mode);
			temp <<= 1;
			y++;							//�������·������ص�				
			if((y - y0)==64)//��ѡ 64, 80, 128
			{
				y = y0;
				x++;
				if(x>=128) return;			//��������
				else;
				break;
			}
			else;
			if(y>=64) break;				//��������
			else;
		}
	}
}

//��ʼ��SSD1306					    
void oled_ssd1306_init(void)//Ĭ�ϲ����� (Page_0,Column_0)��oled���Ͻǵ�λ��
{
	OLED_RST = 0;
	delay_ms(50);
	OLED_RST = 1;
	delay_ms(30);
	
	oled_write_Byte(0xAE,OLED_CMD);	//�ر���ʾ
	oled_write_Byte(0x00,OLED_CMD);	//�����еĵ͵�ַ
	oled_write_Byte(0x10,OLED_CMD);	//�����еĸߵ�ַ
	oled_write_Byte(0x40,OLED_CMD); //������ʾ��ʼ��[5:0],����.
	//oled_write_Byte(0xB0,OLED_CMD);//����ҳ�׵�ַ
	
	oled_write_Byte(0x81,OLED_CMD); //�Աȶ�����
	oled_write_Byte(0x3F,OLED_CMD); //1~255;Ĭ��0X7F(��������,Խ��Խ��)
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
	
	
	oled_write_Byte(0xDB,OLED_CMD); //����VCOMH ��ѹ����
	oled_write_Byte(0x00,OLED_CMD); //0x00Ϊ0.65*VCC; 0x20Ϊ0.77*VCC; 0x30Ϊ0.83*VCC;(��ѹԽ��Խ��)
	oled_write_Byte(0x20,OLED_CMD); //�����ڴ��ַģʽ 
	oled_write_Byte(0x02,OLED_CMD); //0x00ˮƽѰַ,0x01��ֱѰַ,0x02ҳѰַ(Ĭ��)
	oled_write_Byte(0x8D,OLED_CMD); //��ɱ�����
	oled_write_Byte(0x14,OLED_CMD); //�õĿ���/�ر�
	oled_write_Byte(0xA4,OLED_CMD); //0xA4Ϊ��ʾ�ڴ������,0xA5�����ڴ������
	oled_write_Byte(0xAF,OLED_CMD); //������ʾ
	delay_ms(10);
	buffer_clear();
}  



//Plexer�����Ѿ���֤���������д������ȷ��.
