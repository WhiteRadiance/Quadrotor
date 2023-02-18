#include "timer.h"

/*
/	** AHB对SYSCLK进行1分频为72MHz, APB2对AHB进行1分频为72MHz, APB1对AHB2进行2分频为36MHz **
/	** 若APB2不是AHB的1分频, 则TIM1/8会启动2倍频, 也即APB2=18MHz时TIM1的驱动时钟为18*2=36MHz **
/	** 若APB1不是AHB的1分频, 则TIM2-7会启动2倍频, 也即APB1=36MHz时TIM2的驱动时钟为36*2=72MHz **
/	TIM1, TIM8 的时钟是APB2通过一个可激活倍频器, 因为APB2没有分频, 所以TIM1/8也不激活倍频, 还是72MHz
/	TIM2-7 的时钟是APB1+倍频器, 因为APB1将AHB进行了2分频, 所以TIM2-7实际时钟是分频后的2倍频, 即72MHz
*/
						 
//PWM输出初始化
//arr：自动重装值
//psc：时钟预分频数
void TIM4_PWM_Init(u16 arr,u16 psc)
{  
	GPIO_InitTypeDef		GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef	TIM_TimeBaseStructure;	//时间基数 结构体
	TIM_OCInitTypeDef		TIM_OCInitStructure;	//OutputCampare结构体

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);	//APB1_TIM4时钟使能
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//GPIOB时钟使能
	                                                                     	
   //设置该引脚为复用输出功能,输出TIM4_CH1/2/3/4的PWM脉冲波形
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6|GPIO_Pin_7|GPIO_Pin_8|GPIO_Pin_9;	//PA6/7/8/9
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;		//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;	//影响功耗
	GPIO_Init(GPIOB, &GPIO_InitStructure);

	TIM_TimeBaseStructure.TIM_Prescaler = psc;					//设置TIMx的预分频值, 即计数频率
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM4 向上计数模式
	TIM_TimeBaseStructure.TIM_Period = arr;						//设置自动重装载寄存器, 即计数器溢出值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//为定时器的ETRP滤波器提供时钟: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;		//默认
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure);

	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;			//定时器模式为PWM1: 计数值 < 预设值 时输出有效电平
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; 	//比较输出使能
	TIM_OCInitStructure.TIM_OutputNState = TIM_OutputNState_Disable;//默认: 关闭互补输出
	TIM_OCInitStructure.TIM_Pulse = 0; 								//待装入捕获比较寄存器的值, 即溢出值(暂设为0)
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; 		//输出极性: 高电平有效
	TIM_OCInitStructure.TIM_OCNPolarity = TIM_OCNPolarity_High;		//默认
	TIM_OCInitStructure.TIM_OCIdleState = TIM_OCIdleState_Reset;	//默认
	TIM_OCInitStructure.TIM_OCNIdleState = TIM_OCNIdleState_Reset;	//默认
	
	TIM_OC1Init(TIM4, &TIM_OCInitStructure);//注意此行函数只能初始化CH1
	TIM_OC2Init(TIM4, &TIM_OCInitStructure);	//初始化TIM_CH2
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);	//初始化TIM_CH3
	TIM_OC4Init(TIM4, &TIM_OCInitStructure);	//初始化TIM_CH4
	
	
	//TIM_CtrlPWMOutputs(TIM1,ENABLE);					//MOE主输出使能(用于高级定时器)

	TIM_OC1PreloadConfig(TIM4, TIM_OCPreload_Enable);//注意此行函数只能使能TIM_CH1的预装载
	TIM_OC2PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH2预装载使能
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH3预装载使能
	TIM_OC4PreloadConfig(TIM4, TIM_OCPreload_Enable);	//TIM_CH4预装载使能
	
	TIM_ARRPreloadConfig(TIM4, ENABLE);					//使能ARR(自动重载)的预装载
	
	TIM_Cmd(TIM4, ENABLE);
}


//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!

void TIM3_Config_Init(u16 arr,u16 psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;		//timer_init 结构体
	NVIC_InitTypeDef         NVIC_InitStructure;		//NVIC_init 结构体
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);	//时钟使能
	
	TIM_TimeBaseStructure.TIM_Prescaler = psc;					//设置TIMx的预分频值, 即计数频率
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;	//TIM向上计数模式
	TIM_TimeBaseStructure.TIM_Period = arr;						//设置自动重装载寄存器, 即计数器溢出值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//为定时器的ETRP滤波器提供时钟: TDTS = x * Tck_tim
	//TIM_TimeBaseStructure.TIM_RepetitionCounter = 0x00;  		//通用定时器不需要设置此成员变量
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE);		//使能TIM3中断之中的 更新中断

	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;				//TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;	//先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;			//从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);

	TIM_Cmd(TIM3,ENABLE);							//开启(使能)定时器
}




extern u8	Trig_LED_Scan;		// 4Hz
extern u8	Trig_ADC_Battery;	// 2Hz

//TIM3 中断服务程序
void TIM3_IRQHandler(void)
{
	static u16 ms250=0, ms500=0;
	
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)	//检查指定的TIM中断发生与否
	{
		ms250++;
		ms500++;

		if(ms250 >= 250)	// 4Hz
		{
			ms250 = 0;
			Trig_ADC_Battery = 1;
		}
		else ;
		if(ms500 >= 500)	// 2Hz
		{
			ms500 = 0;
			Trig_LED_Scan = 1;
		}
		else ;
	}
	TIM_ClearITPendingBit(TIM3, TIM_IT_Update);			//清除TIM3的中断待处理位
}
