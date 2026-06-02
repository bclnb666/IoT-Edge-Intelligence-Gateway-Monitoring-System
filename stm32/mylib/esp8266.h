#ifndef __ESP8266_H
#define __ESP8266_H
/*
ESP8266模块是WIFI无线通讯模块,该模块是串口接口
连接到了我们开发板上的USART2
*/
#include "stm32f10x_conf.h"//包含该头文件所有的外设头文件均已包含

typedef void (*wifi_recv_handler)(char *buf, int len);//通过typedef定义函数指针类型

extern void set_wifi_recv_handler(wifi_recv_handler h);//设置wifi接收到数据的回调函数
extern void esp8266_init(void);//初始化的函数(初始化串口)
extern void esp8266_link_wifi(char *ssid, char *passwd);//连接路由(需要路由的名称和路由的密码)
extern void esp8266_connect(char *ip, char *port);//连接TCP server
extern void esp8266_disconnect(void);//断开TCP连接
extern void esp8266_send(char *msg, char *len);//发送数据
extern void dataHandler(unsigned char c);//处理接收到的数据

#endif









