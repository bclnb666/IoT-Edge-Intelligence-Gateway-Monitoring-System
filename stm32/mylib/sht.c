#include "sht.h"
#include "gpio_iic.h"

void sht_init(void)//初始化SHT30传感器使用的管脚
{
	IIC_Init();//调用GPIO模拟IIC协议的初始化代码
}

void sht_write_mode(void)//给SHT30传感器写一个采样指令
{
	IIC_Start();//产生开始信号
	IIC_Send_Byte(SLAVE_ADDR);//发送设备地址 + 写的标志
	while(IIC_Wait_Ack());//等待ACK
	IIC_Send_Byte(PERIODIC_MODE_HIGH >> 8);//发送采样指令的高字节
	while(IIC_Wait_Ack());//等待ACK
	IIC_Send_Byte(PERIODIC_MODE_HIGH & 0xFF);//发送采样指令的低字节
	while(IIC_Wait_Ack());//等待ACK
	IIC_Stop();//产生结束信号
}

void sht_write_read_cmd(void)//给SHT30传感器写一个读取数据指令
{
	IIC_Start();//产生开始信号
	IIC_Send_Byte(SLAVE_ADDR);//发送设备地址 + 写的标志
	while(IIC_Wait_Ack());//等待ACK
	IIC_Send_Byte(FETCH_DATA >> 8);//发送读取数据指令的高字节
	while(IIC_Wait_Ack());//等待ACK
	IIC_Send_Byte(FETCH_DATA & 0xFF);//发送读取数据指令的低字节
	while(IIC_Wait_Ack());//等待ACK
}

void sht_read_data(double sht_data[2])//获取SHT30传感器数据以及转换成温度和湿度
{
	u8 i = 0;//循环变量
	u8 buf[6] = {0};//存储从SHT30读取出的6byte数据
	u16 temp = 0;//存储温度的高字节 + 低字节
	u16 hum = 0;//存储湿度的高字节 + 低字节
	
 	IIC_Start();//产生开始信号
	IIC_Send_Byte(SLAVE_ADDR | 1);//发送设备地址 + 读的标志
	while(IIC_Wait_Ack());//等待ACK
	for(i = 0; i < 6; i++)
	{
		if(i == 5)//判断是否是读取最后一个字节数据
			buf[i] = IIC_Recv_Byte(0);//读取最后一个字节数据,并且不产生ACK
		else
			buf[i] = IIC_Recv_Byte(1);//读取之前字节的数据,并且产生ACK
	}
	IIC_Stop();//产生结束信号
	
	temp = buf[0] & 0xFF;//存储温度的高字节
	temp <<= 8;//把温度的高字节数据挪到高8bit
	temp |= buf[1];//存储温度的低字节
	hum = buf[3] & 0xFF;//存储湿度的高字节
	hum <<= 8;//把湿度的高字节数据挪到高8bit
	hum |= buf[4];//存储湿度的低字节
	
	sht_data[0] = (double)(-45 + 175*(double)(temp) / 65535);
	sht_data[1] = (double)(100 * (double)(hum) / 65535);
}









