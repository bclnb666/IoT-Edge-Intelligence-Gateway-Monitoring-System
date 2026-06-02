#include "led.h"
#include "buzzer.h"
#include "delay.h"
#include "esp8266.h"
#include "iot.h"
#include "string.h"
#include "sht30.h"

#define SIZE 2
#define API_KEY "eyJpYXQiOjE3NzMyODU5MjcsImV4cCI6MjA4ODY0NTkyNywiYWxnIjoiSFMyNTYifQ.eyJpZCI6NTgwMH0.soaHZPJcacDLoUd3nsXva5U44WlBNfQcxlC8L_9uRiE:"
//API_KEY需要修改成自己账号(保留冒号)


void recv_handler(char *buf, int len)
{
	char *v = NULL;
	
	v = strstr(buf, "true");
	if(v != NULL)
		buzzer_on();
	else
		buzzer_off();
}


int main(void)
{
	double sht_data[SIZE] = {0};//存储温度湿度
	int sensor_id_arrary[SIZE] = {16790,16789};//需要修改成自己的传感器的ID号
	//double sensor_data_arrary[SIZE] = {22, 33};//假数据
	
	
	sht_init();//调用初始化SHT30的函数
	led_init();
	buzzer_init();
	delay_init();
	esp8266_init();
	
	sht_write_mode();//设置SHT30传感器的采样频率
	
	
	led_on(0);
	esp8266_link_wifi("Amon", "Praisethefool");//需要修改成自己的手机热点
	delay_ms(500);
	led_off(0);
	
	set_wifi_recv_handler(recv_handler);
	
	while(1)
	{
		sht_write_read_cmd();//发送获取SHT30数据的指令
		sht_read_data(sht_data);//获取SHT30采集的数据
		
		//sensor_data_arrary[0]++;
		//sensor_data_arrary[1]++;
		upload_device_datas("119.29.98.16", "5910", API_KEY, sensor_id_arrary, sht_data, SIZE);
		//IP地址不需要修改
		led_on(1);
		delay_ms(400);
		led_off(1);
		delay_ms(400);
		download_sensor_status("119.29.98.16", "5910", "16666", API_KEY);
		led_on(2);
		delay_ms(400);
		led_off(2);
		delay_ms(400);
	}
}


