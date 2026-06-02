#ifndef __LDT_H
#define __LDT_H
/*
HC138_A0    PC11
HC138_A1    PC12
HC138_A2    PD2
HC595_SCLK  PB5
HC595_LCLK  PB4
HC595_DATA  PB3
需要把这些管脚初始化为推挽输出
*/
#include "stm32f10x_conf.h"

extern void ldt_init(void);//初始化数码管使用的GPIO管脚

extern void hc138_out_data(u8 data);//通过74HC138模块选择数码管的公共端

extern void hc595_send_data(u8 data);//通过74HC595模块把串行的编码转换成并行数据

extern void digit_show_data(int data);//把形参的数字通过四位数码管显示

#endif





