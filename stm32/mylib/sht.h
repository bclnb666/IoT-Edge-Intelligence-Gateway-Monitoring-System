#ifndef __SHT_H
#define __SHT_H

#include "stm32f10x_conf.h"

#define SLAVE_ADDR 0x88//设备地址
#define PERIODIC_MODE_HIGH 0X2737//采样指令
#define FETCH_DATA 0xE000//获取数据指令

extern void sht_init(void);//初始化SHT30传感器使用的管脚

extern void sht_write_mode(void);//给SHT30传感器写一个采样指令

extern void sht_write_read_cmd(void);//给SHT30传感器写一个读取数据指令

extern void sht_read_data(double sht_data[2]);//获取SHT30传感器数据以及转换成温度和湿度

#endif








