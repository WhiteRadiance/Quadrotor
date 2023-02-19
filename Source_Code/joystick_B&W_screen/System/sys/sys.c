#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK  STM32开发板
//系统中断分组设置化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/10
//版本：V1.4
//版权所有，盗版必究。
//Copyright(C) 正点原子 2009-2019
//All rights reserved
//********************************************************************************  
//THUMB指令不支持汇编内联
//采用如下方法实现执行汇编指令WFI  
void WFI_SET(void)
{
	__ASM volatile("wfi");
}
//关闭所有中断
void INTX_DISABLE(void)
{		  
	__ASM volatile("cpsid i");
}
//开启所有中断
void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");
}

//设置栈顶地址
//addr:栈顶地址
__asm void MSR_MSP(u32 addr) 
{
    MSR MSP, r0 			//set Main Stack value
    BX r14
}
/*
//设置栈顶地址
//addr:栈顶地址
void MSR_MSP(u32 addr) 
{
    __ASM volatile("MSR MSP, r0 ");		//set Main Stack value
    __ASM volatile("BX r14");
}
*/
//设置NVIC中断分组2:2位抢占优先级，2位响应优先级。
//最新版本SYSTEM删除了设置中断分组函数，如果需要使用中断的话，需要自行在主程序开头添加如下函数.

//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	

/* 
 The table below gives the allowed values of the pre-emption priority and subpriority according
 to the Priority Grouping configuration performed by NVIC_PriorityGroupConfig function
  ====================================================================================================================
    NVIC_PriorityGroup   | 		抢占优先级级别		| 		相应优先级级别	   | 	   分别占用几个bit
  ====================================================================================================================
   NVIC_PriorityGroup_0  |            0             |            0-15          |   0 bits for pre-emption priority
                         |                          |                          |   4 bits for subpriority
  --------------------------------------------------------------------------------------------------------------------
   NVIC_PriorityGroup_1  |           0-1            |            0-7           |   1 bits for pre-emption priority
                         |                          |                          |   3 bits for subpriority
  --------------------------------------------------------------------------------------------------------------------    
   NVIC_PriorityGroup_2  |           0-3            |            0-3           |   2 bits for pre-emption priority
                         |                          |                          |   2 bits for subpriority
  --------------------------------------------------------------------------------------------------------------------    
   NVIC_PriorityGroup_3  |           0-7            |            0-1           |   3 bits for pre-emption priority
                         |                          |                          |   1 bits for subpriority
  --------------------------------------------------------------------------------------------------------------------    
   NVIC_PriorityGroup_4  |           0-15           |            0             |   4 bits for pre-emption priority
                         |                          |                          |   0 bits for subpriority                       
  =====================================================================================================================
*/
