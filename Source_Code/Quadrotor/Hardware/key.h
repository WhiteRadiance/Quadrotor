#ifndef _KEY_H
#define _KEY_H

#include "sys.h"//��Ҫ�������ͷ�ļ�

//λ�����汾(��Ҫ����sys.h)
//#define key0 		PCin(5)   	
//#define key1 		PAin(15)	 
//#define wk_up  	PAin(0)

//�⺯���汾
/*
#define key0	GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)	//����ģʽ�£���ȡ����0
#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)//��ȡ����1
#define wk_up 	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)	//��ȡ����2
*/
#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)	//��ȡ����1
//#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)	//��ȡ����1
//#define key2	GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)	//��ȡ����2
//#define key3 	GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9 )	//��ȡ����3, key_user

//����3����������ʱ��key_scan�ķ���ֵ
/*
#define key0_press 	1		//key0
#define key1_press 	2 		//key1
#define wkup_press 	3 		//wkup
*/
#define key1_press 	1		//key1
//#define key2_press 	2 		//key2
//#define key3_press 	3 		//key3

void key_init(void);	//������ʼ��
u8 key_scan(u8 mode);	//����ɨ�躯��


#endif
