#ifndef __USART1_H
#define __USART1_H
/*
U1_TX(发送)		PA9
U1_RX(接收)		PA10
PA9管脚配置为推挽的复用模式
PA10管脚配置为浮空的输入模式
*/
#include "stm32f10x_conf.h"

typedef void (*usart1_handler)(u8);//定义了函数指针类型

extern void usart_1_init(void);//初始化USART1使用的管脚

extern void usart_1_send_byte(u8 data);//通过USART1发送1byte数据

extern void usart_1_send_data(u8 *buf);//通过USART1发送多个字节数据

extern u8 usart_1_recv_byte(void);//通过USART1接收1byte数据

extern void set_usart1_handler(usart1_handler h);//设置回调函数

#endif






