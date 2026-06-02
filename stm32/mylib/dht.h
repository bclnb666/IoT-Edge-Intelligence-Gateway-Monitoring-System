#ifndef __DHT_H
#define __DHT_H
/*
DHT11    PC10
PC10既要输入模式也要输出模式
输入模式为浮空输入
输出模式为推挽输出
*/
#include "stm32f10x_conf.h"

extern void dht_init(void);//初始化DHT11

extern void get_dht_value(u8 data[5]);//获取DHT11采集的数据

#endif








