#include "esp8266.h"
#include "base64.h"
#include "stdio.h"
#include "delay.h"
#include "iot.h"

int upload_sensor_data(const char *ip, \
	const char *device, const char *sensor, \
		const char *id, double data)
{
	int ret = 0;
	char buf[32] = {0};
	char send_data[512] = {0};
	char len_buf[32] = {0};
	char base_id[256] = {0};
	
	ret = sprintf(buf, "%f", data);
	base64_encode((unsigned char *)base_id, (const unsigned char *)id);
	ret = sprintf(send_data, "POST /api/1.0/device/%s/sensor/%s/data HTTP/1.1\r\nHost: www.embsky.com\r\nAccept: */*\r\nAuthorization: Basic %s\r\nContent-Length: %d\r\nContent-Type: application/json;charset=utf-8\r\nConnection: close\r\n\r\n{\"data\":%f}\r\n", device, sensor, base_id, ret + 10, data);

	esp8266_connect((char *)ip, "80");
	sprintf(len_buf, "%d", ret);
	delay_ms(200);
	esp8266_send(send_data, len_buf); 
	delay_ms(500);
	esp8266_disconnect();

	return 0;
}

int upload_sensor_status(const char *ip, \
	const char *device, const char *sensor, \
		const char *id, char *status)
{
	int ret = 1;
	char send_data[512] = {0};
	char len_buf[32] = {0};
	char base_id[256] = {0};
	
	base64_encode((unsigned char *)base_id, (const unsigned char *)id);
	ret = sprintf(send_data, "POST /api/1.0/device/%s/sensor/%s/data HTTP/1.1\r\nHost: www.embsky.com\r\nAccept: */*\r\nAuthorization: Basic %s\r\nContent-Length: %d\r\nContent-Type: application/json;charset=utf-8\r\nConnection: close\r\n\r\n{\"data\":\"%s\"}\r\n", device, sensor, base_id, ret + 1 + 10, status);
	
	esp8266_connect((char *)ip, "80");
	sprintf(len_buf, "%d", ret);
	delay_ms(200);
	esp8266_send(send_data, len_buf); 
	delay_ms(500);
	esp8266_disconnect();

	return 0;
}

int upload_device_datas(const char *ip, \
	const char *device, const char *id, \
		int sensor_id_arrary[], double sensor_data_arrary[], int size)
{
	int i = 0;
	int ret = 0;
	char send_data[512] = {0};
	char data_list[128] = {0};
	char len_buf[32] = {0};
	char base_id[256] = {0};
	
	ret = sprintf(data_list, "%s", "[");
	
	for(i = 0; i < size; i++)
	{
		if(i == size - 1)
		{
			ret += sprintf(data_list + ret, "{\"id\":%d, \"data\":%f}", sensor_id_arrary[i], sensor_data_arrary[i]); 
		}
		else
		{
			ret += sprintf(data_list + ret, "{\"id\":%d, \"data\":%f},", sensor_id_arrary[i], sensor_data_arrary[i]);
		}
	}
	ret += sprintf(data_list + ret, "%s", "]");

	base64_encode((unsigned char *)base_id, (const unsigned char *)id);
	ret = sprintf(send_data, "POST /api/1.0/device/%s/datas HTTP/1.1\r\nHost: www.embsky.com\r\nAccept: */*\r\nAuthorization: Basic %s\r\nContent-Length: %d\r\nContent-Type: application/json;charset=utf-8\r\nConnection: close\r\n\r\n{\"datas\":%s}\r\n", device, base_id, ret + 10, data_list);
	
	esp8266_connect((char *)ip, "80");
	sprintf(len_buf, "%d", ret);
	delay_ms(200);
	esp8266_send(send_data, len_buf); 
	delay_ms(500);
	esp8266_disconnect();
	
	return 0;
}

int download_sensor_status(const char *ip, \
	const char *device, const char *sensor, \
		const char *id)
{
	int ret = 0;
	char send_data[512] = {0};
	char len_buf[32] = {0};
	char base_id[256] = {0};

	base64_encode((unsigned char *)base_id, (const unsigned char *)id);
	ret = sprintf(send_data, "GET /api/1.0/device/%s/sensor/%s/data HTTP/1.1\r\nHost: www.embsky.com\r\nAccept: */*\r\nAuthorization: Basic %s\r\nContent-Length: 0\r\nConnection: close\r\n\r\n", device, sensor, base_id);

	esp8266_connect((char *)ip, "80");//½¨Á¢Á´½Ó
	sprintf(len_buf, "%d", ret);
	esp8266_send(send_data, len_buf);
	delay_ms(300);
	esp8266_disconnect();

	return 0;
}

