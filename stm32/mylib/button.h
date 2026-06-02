#ifndef __BUTTON_H
#define __BUTTON_H
/*
KEY0	PC9
KEY1	PC8
KEY2	PA0
PC9 PC8配置为上拉输入模式
PA0配置为下拉输入模式
*/
#include "stm32f10x_conf.h"

extern void button_init(void);//初始化功能按键使用的GPIO管脚

extern int button_status(int num);//根据形参检测相应按键的电平高低

#endif








