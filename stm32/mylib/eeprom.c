#include "eeprom.h"
#include "gpio_iic.h"

void eeprom_init(void)//初始化EEPROM的函数
{
	IIC_Init();//调用GPIO模拟IIC的初始化函数
}

void eeprom_byte_write(u8 daddr, u8 waddr, u8 data)//按字节写
{
	IIC_Start();//产生开始信号
	IIC_Send_Byte(daddr);//发送设备地址 + 写的标志
	IIC_Wait_Ack();//等待ACK
	IIC_Send_Byte(waddr);//发送内部存储空间地址
	IIC_Wait_Ack();//等待ACK
	IIC_Send_Byte(data);//发送1byte数据
	IIC_Wait_Ack();//等待ACK
	IIC_Stop();//产生结束信号
}

void eeprom_page_write(u8 daddr, u8 waddr, u8 *data, u8 size)//按页写
{
}

u8 eeprom_curr_read(u8 daddr)//当前地址读
{
	return 0;
}

u8 eeprom_rand_read(u8 daddr, u8 waddr)//随机地址读
{
	u8 data = 0;//存储从EEPROM中读取出的数据
	
	IIC_Start();//产生开始信号
	IIC_Send_Byte(daddr);//发送设备地址 + 写的标志
	IIC_Wait_Ack();//等待ACK
	IIC_Send_Byte(waddr);//发送内部存储空间地址
	IIC_Wait_Ack();//等待ACK
	IIC_Start();//产生开始信号
	IIC_Send_Byte(daddr | 0x01);//发送设备地址 + 读的标志
	IIC_Wait_Ack();//等待ACK
	data = IIC_Recv_Byte(0);//读取1byte数据并且不产生ACK
	IIC_Stop();//产生结束信号
	
	return data;
}

void eeprom_sequ_read(u8 daddr, u8 waddr, u8 *data, u8 size)//顺序读
{
}





