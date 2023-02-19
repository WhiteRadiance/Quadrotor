#ifndef _WKUP_H
#define _WKUP_H

#include "sys.h"

#define		wk_up 	PAin(0)		//PA0,检测是否外部WK_UP按键按下

u8 check_wkup(void);//检测WKUP脚的信号
void wkup_init(void);//PA0 WKUP唤醒初始化
void sys_enter_standby(void);//系统进入待机模式

#endif
