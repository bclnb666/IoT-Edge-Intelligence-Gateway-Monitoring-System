#ifndef __LED_H
#define __LED_H
/*
LED0	PC1
LED1	PC2
LED2	PC3
输出高电平,灯亮
输出低电平,灯灭
*/
#include "stm32f10x_conf.h"//包含这一个头文件,所有外设的头文件均包含了

extern void led_init(void);//初始化LED使用的GPIO管脚

extern void led_on(int num);//通过形参num点亮相应的LED灯

extern void led_off(int num);//通过形参num灭掉相应的LED灯

#endif






