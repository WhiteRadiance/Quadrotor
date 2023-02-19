#include "sys.h"

//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//ALIENTEK  STM32������
//ϵͳ�жϷ������û�		   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2012/9/10
//�汾��V1.4
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved
//********************************************************************************  
//THUMBָ�֧�ֻ������
//�������·���ʵ��ִ�л��ָ��WFI  
void WFI_SET(void)
{
	__ASM volatile("wfi");
}
//�ر������ж�
void INTX_DISABLE(void)
{		  
	__ASM volatile("cpsid i");
}
//���������ж�
void INTX_ENABLE(void)
{
	__ASM volatile("cpsie i");
}

//����ջ����ַ
//addr:ջ����ַ
__asm void MSR_MSP(u32 addr) 
{
    MSR MSP, r0 			//set Main Stack value
    BX r14
}
/*
//����ջ����ַ
//addr:ջ����ַ
void MSR_MSP(u32 addr) 
{
    __ASM volatile("MSR MSP, r0 ");		//set Main Stack value
    __ASM volatile("BX r14");
}
*/
//����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ���
//���°汾SYSTEMɾ���������жϷ��麯���������Ҫʹ���жϵĻ�����Ҫ������������ͷ������º���.

//NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);	

/* 
 The table below gives the allowed values of the pre-emption priority and subpriority according
 to the Priority Grouping configuration performed by NVIC_PriorityGroupConfig function
  ====================================================================================================================
    NVIC_PriorityGroup   | 		��ռ���ȼ�����		| 		��Ӧ���ȼ�����	   | 	   �ֱ�ռ�ü���bit
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
