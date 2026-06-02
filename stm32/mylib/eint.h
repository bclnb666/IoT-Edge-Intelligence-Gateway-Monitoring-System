#ifndef __EINT_H
#define __EINT_H
/*
功能按键	管脚	EXTI号	中断号
KEY0			PC9		EXTI9		EXTI9_5
KEY1			PC8		EXTI8		EXTI9_5
KEY2			PA0		EXTI0		EXTI0
*/
#include "stm32f10x_conf.h"

typedef void (*eint_handler)(void);//定义了函数指针类型

extern void eint_init(void);//初始化功能按键使用的GPIO管脚和外部中断以及终端控制器

extern void set_eint_handler(eint_handler h0, eint_handler h1, eint_handler h2);//设置回调函数

#endif






