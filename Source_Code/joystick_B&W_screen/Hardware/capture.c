#include "capture.h"
#include "led.h"
#include "usart.h"
#include "sys.h"
//第一个函数是:复制pwm.c的 PWM输出函数
//第二个函数是:更有意义的  输入捕获函数


	//PWM输出初始化		//arr：自动重装值		//psc：时钟预分频数
void TIM1_PWM_init(u16 arr,u16 psc)
{	//3个初始化结构体
	GPIO_InitTypeDef 					GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  	TIM_TimeBaseStructure;
	TIM_OCInitTypeDef 			 	TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);//使能定时器1的时钟
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA , ENABLE);//使能GPIOA外设时钟使能
	                                                                     	

  //设置该引脚为复用输出功能,输出TIM1 CH1(PA8复用)的PWM脉冲波形
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;//TIM_CH1(PA8复用)
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;//复用推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	
	TIM_TimeBaseStructure.TIM_Period = arr;//设置在下一个更新事件装入活动的自动重装载寄存器周期的值	 80K
	TIM_TimeBaseStructure.TIM_Prescaler =psc;//设置用来作为TIMx时钟频率除数的预分频值  不分频
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;//设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM向上计数模式
	TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);//根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位

 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM2;//选择定时器模式:TIM脉冲宽度调制模式2
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;//比较输出使能
	TIM_OCInitStructure.TIM_Pulse = 0;//设置待装入捕获比较寄存器的脉冲值
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;//输出极性:与比较值匹配后,TIM输出极性为高
	TIM_OC1Init(TIM1, &TIM_OCInitStructure);//根据TIM_OCInitStruct中指定的参数初始化外设TIMx的OC1:输出通道1

  TIM_CtrlPWMOutputs(TIM1,ENABLE);//MOE 主输出使能	

	TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);//CH1预装载使能	 
	
	TIM_ARRPreloadConfig(TIM1, ENABLE);//使能TIMx在ARR上的预装载寄存器
	
	TIM_Cmd(TIM1, ENABLE);//使能TIM1
}


//定时器2通道1输入捕获配置
TIM_ICInitTypeDef  TIM2_ICInitStructure;
void TIM2_Cap_init(u16 arr,u16 psc)
{	 
	//TIM_ICInitTypeDef  TIM2_ICInitStructure;//*********************************************
	
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);//使能TIM2时钟
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);//使能GPIOA时钟
	
	GPIO_InitStructure.GPIO_Pin  = GPIO_Pin_0;//PA0 清除之前设置  
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;//PA0 下拉输入  
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	GPIO_ResetBits(GPIOA,GPIO_Pin_0);
	
	//初始化定时器2 TIM2	 
	TIM_TimeBaseStructure.TIM_Period = arr;//设定计数器自动重装值 
	TIM_TimeBaseStructure.TIM_Prescaler =psc;//预分频器   
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;//设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;//TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);//根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
  
	//初始化TIM2输入捕获参数
	TIM2_ICInitStructure.TIM_Channel = TIM_Channel_1;//CC1S=01,选择输入端,IC1映射到TI1上
	TIM2_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;//上升沿捕获
	TIM2_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI;//映射到TI1上
	TIM2_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;//配置输入分频,不分频 
	TIM2_ICInitStructure.TIM_ICFilter = 0x00;//IC1F=0000,配置输入滤波器,不滤波
	TIM_ICInit(TIM2, &TIM2_ICInitStructure);
	
	//中断分组初始化
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;//TIM2中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//先占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;//IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);//根据NVIC_InitStruct中指定的参数初始化外设NVIC寄存器 
	
	TIM_ITConfig(TIM2,TIM_IT_Update|TIM_IT_CC1,ENABLE);//允许更新中断 ,允许CC1IE捕获中断	
																		//因为脉宽较长时定时器可能溢出，所以要允许更新中断
  TIM_Cmd(TIM2,ENABLE );//使能定时器2
}


//定时器5中断服务程序
//第一步进入捕获上升沿的else语句,可能执行更新中断,最后又捕获一次下降沿
u8  TIM2CH1_CAPTURE_STA = 0;//输入捕获状态		    				
u16	TIM2CH1_CAPTURE_VAL;//输入捕获值
void TIM2_IRQHandler(void)
{ 

 	if((TIM2CH1_CAPTURE_STA & 0x80)==0)//还未成功捕获	
	{	  
			if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)//发生更新中断
			{	    
					if(TIM2CH1_CAPTURE_STA & 0x40)//已经捕获到高电平了
					{
							if((TIM2CH1_CAPTURE_STA & 0x3F)==0x3F)//高电平太长了,强制认为高电平已经结束
							{
									TIM2CH1_CAPTURE_STA |= 0x80;//标记成功捕获了一次
									TIM2CH1_CAPTURE_VAL = 0xFFFF;
							}
							else
									TIM2CH1_CAPTURE_STA++;
					}	 
			}
			if(TIM_GetITStatus(TIM2, TIM_IT_CC1) != RESET)//捕获1发生捕获事件,可能是上升沿,也可能是下降沿
			{	
					if(TIM2CH1_CAPTURE_STA & 0x40)//是否已经捕获过了一个上升沿,即当前捕获的就是下降沿
					{	  			
							TIM2CH1_CAPTURE_STA |= 0x80;//标记成功捕获到一次 高电平脉宽
							TIM2CH1_CAPTURE_VAL = TIM_GetCapture1(TIM2);//读取捕获值
							TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Rising);//CC1P=0,设置为上升沿捕获
					}
					else//还未开始,第一次捕获上升沿
					{
							TIM2CH1_CAPTURE_STA = 0;//清空
							TIM2CH1_CAPTURE_VAL = 0;
							TIM_SetCounter(TIM2,0);
							TIM2CH1_CAPTURE_STA |= 0x40;		//标记捕获到了上升沿
							TIM_OC1PolarityConfig(TIM2,TIM_ICPolarity_Falling);//CC1P=1,设置为下降沿捕获
					}		    
			}			     	    					   
 	}
	TIM_ClearITPendingBit(TIM2, TIM_IT_CC1|TIM_IT_Update);//清除中断标志位
}
