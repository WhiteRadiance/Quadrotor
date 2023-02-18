#ifndef _KEY_H
#define _KEY_H

#include "sys.h"//不要忘了这个头文件

//位操作版本(需要调用sys.h)
//#define key0 		PCin(5)   	
//#define key1 		PAin(15)	 
//#define wk_up  	PAin(0)

//库函数版本
/*
#define key0	GPIO_ReadInputDataBit(GPIOC,GPIO_Pin_5)	//输入模式下，读取按键0
#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)//读取按键1
#define wk_up 	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_0)	//读取按键2
*/
#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_1)	//读取按键1
//#define key1	GPIO_ReadInputDataBit(GPIOA,GPIO_Pin_15)	//读取按键1
//#define key2	GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_11)	//读取按键2
//#define key3 	GPIO_ReadInputDataBit(GPIOB,GPIO_Pin_9 )	//读取按键3, key_user

//定义3个按键按下时的key_scan的返回值
/*
#define key0_press 	1		//key0
#define key1_press 	2 		//key1
#define wkup_press 	3 		//wkup
*/
#define key1_press 	1		//key1
//#define key2_press 	2 		//key2
//#define key3_press 	3 		//key3

void key_init(void);	//按键初始化
u8 key_scan(u8 mode);	//按键扫描函数


#endif
