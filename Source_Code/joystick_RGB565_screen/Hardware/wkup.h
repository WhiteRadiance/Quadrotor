#ifndef _WKUP_H
#define _WKUP_H

#include "sys.h"

#define		wk_up 	PAin(0)		//PA0,����Ƿ��ⲿWK_UP��������

u8 check_wkup(void);//���WKUP�ŵ��ź�
void wkup_init(void);//PA0 WKUP���ѳ�ʼ��
void sys_enter_standby(void);//ϵͳ�������ģʽ

#endif
