#include "key.h"
#include "delay.h"

//key0---PC5    key1---PA15(��JTAG����,���ڲ�Ĭ��һ������������)    wk_up---PA0
//key0,1�ⲿ�ӵ�(���͵�ƽ������,��������ӦΪ��,��ѡ������ģʽ); wk_up�ⲿ��3V3(���ѡ������)


//�ɻ���:  key1---PA1
void key_init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA/* | RCC_APB2Periph_GPIOB*/, ENABLE);//ʹ��GPIOA,B��ʱ��
	
	/* �ر�JTAG, ����SWD����, ��A15��Ϊ��ͨIO */
	//GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);
	
	
	
	//����Ϊ����ʱ����Ϊ���������·��˿��ǶϿ��ģ����� ��������ٶ�������
	
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//����
	GPIO_Init(GPIOA, &GPIO_InitStructure);			//PA1
	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//����
//	GPIO_Init(GPIOB, &GPIO_InitStructure);			//PB11
//	
//	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
//	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;	//����
//	GPIO_Init(GPIOB, &GPIO_InitStructure);			//PB9
	
}

//����ĺ������ص��ǰ���ֵ
//mode: 	0,��֧�ֳ���; 1,֧�ֳ���
//����ֵ: 	0,�ް�������; KEY0_PRES,KEY0����; KEY1_PRES,KEY1����; WKUP_PRES,WK_UP����
//ע��˺�������Ӧ���ȼ�:KEY0 > KEY1 > WK_UP !!!
u8 key_scan(u8 mode)
{
	//��ʹ���ϵ��ô˺���,��̬����staticҲֻ��ʼ��һ�Σ�����������ͬ����
	static u8 key_up = 1;	//���� �ɿ���־
	
	if(mode)	key_up = 1;	//1Ϊ֧�ֳ���,��Ч��ֹ���Ų��ſ������ּ��(while)��
	
	if(key_up && (key1==0/*||key2==0||key3==0*/) )	//����Ч
	{
		delay_ms(8);//ȥ��
		key_up = 0;
		if(key1==0)			return key1_press;	//return 1
		/*
		else if(key2==0)	return key2_press;	//return 2
		else if(key3==0)	return key3_press;	//return 3
		*/
	}
	else if( key1==1 /*&& key2==1 && key3==1 */)	//�ް�������
		key_up = 1;
	return 0;
}
