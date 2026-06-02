#ifndef __IWDG_H
#define __IWDG_H
/*
驱动独立看门狗
时钟频率 : LSI(40KHz) / 分频系数(4 8 16 32 64 128 256)
计数值范围 : 12bit的计数器[0 ~ 4095]
*/
#include "stm32f10x_conf.h"

extern void iwdg_init(int ms);//初始化IWDG

extern void iwdg_feed_dog(void);//喂狗

#endif





