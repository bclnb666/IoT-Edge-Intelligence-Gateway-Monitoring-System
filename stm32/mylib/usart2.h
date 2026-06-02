#ifndef __USART2_H
#define __USART2_H
/*
USART2_TX		PA2
USART2_RX		PA3
已知芯片的管脚是多功能复用型的管脚
输入模式 输出模式 复用模式 模拟模式
我们在配置串口的管脚时，选择复用的模式
USART2_TX		发送	复用模式
USART2_RX		接收	浮空的输入模式
*/
#include "stm32f10x_conf.h"//包含该头文件所有的外设头文件均已包含

typedef void (*usart2_handler)(unsigned char);//通过typedef定义了一个函数指针类型

extern void usart_2_init(void);//初始化的函数(初始化GPIO管脚和串口功能)

extern void usart_2_send_byte(unsigned char c);//发送一个字节的数据

extern void usart_2_send_data(char *buf);//发送一个字符串的数据

extern unsigned char usart_2_recv_byte(void);//接收一个字节数据

extern void set_usart2_handler(usart2_handler h);//设置回调函数

#endif






