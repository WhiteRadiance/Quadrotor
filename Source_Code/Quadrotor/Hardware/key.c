#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(与JTAG共用,且内部默认一定有上拉电阻)    wk_up---PA0
//key0,1外部接地(即低电平代表按下,即无输入应为高,即选择上拉模式); wk_up外部接3V3(因此选择下拉)


//飞机上:  key1---PA1
void key_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA/* | RCC_APB2Periph_GPIOB*/, ENABLE);//使能GPIOA,B的时钟
	
	/* 关闭JTAG, 启用SWD调试, 让A15作为普通IO */
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	
	
	//配置为输入时，因为输出驱动电路与端口是断开的，所以 配置输出速度无意义
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//PA1
	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉
//	GPIO_Init(GPIOB, &GPIO_InitStructure);			//PB11
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//上拉
//	GPIO_Init(GPIOB, &GPIO_InitStructure);			//PB9
	
}

//下面的函数返回的是按键值
//mode: 	0,不支持长按; 1,支持长按
//返回值: 	0,无按键按下; KEY0_PRES,KEY0按下; KEY1_PRES,KEY1按下; WKUP_PRES,WK_UP按下
//注意此函数有响应优先级:KEY0 > KEY1 > WK_UP !!!
u8 key_scan(u8 mode)
{
	//即使不断调用此函数,静态变量static也只初始化一次，与整个程序同周期
	static u8 key_up = 1;	//按键 松开标志
	
	if(mode)	key_up = 1;	//1为支持长按,有效防止按着不放卡在松手检测(while)里
	
	if(key_up && (key1==0/*||key2==0||key3==0*/) )	//低有效
	{
		delay_ms(8);//去抖
		key_up = 0;
		if(key1==0)			return key1_press;	//return 1
		/*
		else if(key2==0)	return key2_press;	//return 2
		else if(key3==0)	return key3_press;	//return 3
		*/
	}
	else if( key1==1 /*&& key2==1 && key3==1 */)	//无按键按下
		key_up = 1;
	return 0;
}
