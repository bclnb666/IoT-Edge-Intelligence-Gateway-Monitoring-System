#include "led.h"
#include "delay.h"
#include "usart2.h"
#include "stdio.h"
#include "stdlib.h"
#include "esp8266.h"
#include "string.h"
#include "dht.h"        // 引入DHT11传感器头文件

// =================== 配置区 ===================
#define WIFI_SSID       "Amon"        // 你的WiFi名称
#define WIFI_PWD        "praisethefool"             // 你的WiFi密码

// UDP组播配置，需与Linux网关端保持一致
#define MULTICAST_IP    "224.1.2.3"       
#define MULTICAST_PORT  "8081"            

// [修改] 将字符串设备ID改为与云平台/网关一致的数字传感器ID
#define TEMP_SENSOR_ID  16790  // 温度传感器ID
#define HUM_SENSOR_ID   16789  // 湿度传感器ID
// ==============================================

// 自定义函数：专门用于UDP组播的连接函数
void esp8266_connect_udp_multicast(void)
{
    char cmd[64];
    
    // 先发送关闭指令，以防之前有残留的连接
    usart_2_send_data("AT+CIPCLOSE\r\n");
    delay_ms(500);
    
    // 构建建立 UDP 传输的 AT 指令: AT+CIPSTART="UDP","224.1.2.3",8081
    sprintf(cmd, "AT+CIPSTART=\"UDP\",\"%s\",%s\r\n", MULTICAST_IP, MULTICAST_PORT);
    usart_2_send_data(cmd);
    delay_ms(2000); // 延时等待ESP8266模块内部Socket资源配置完成
}

// [修改] 统一发送格式为网关能解析的 {"id":xxx, "data":xxx}
void send_sensor_data(uint32_t sensor_id, float data_val)
{
    char buf[128];
    char len[10];
    
    // 构建标准的 JSON 格式数据流
    sprintf(buf, "{\"id\":%u, \"data\":%.1f}\r\n", sensor_id, data_val);
    sprintf(len, "%d", strlen(buf));
    
    // 调用ESP8266发送数据
    esp8266_send(buf, len);
    
    // 数据发送时的 LED 闪烁指示
    led_on(0);
    delay_ms(50);
    led_off(0);
}

int main(void)
{    
    int count = 0;
    u8 dht_data[5] = {0}; 
    float current_temp = 0.0;
    float current_hum = 0.0;
    
    // 硬件初始化
    led_init();
    delay_init();
    usart_2_init();
    esp8266_init();
    dht_init();           
    
    // 1. 连接WiFi 
    led_on(0);
    esp8266_link_wifi(WIFI_SSID, WIFI_PWD);
    delay_ms(2000);
    led_off(0);
    
    // 2. 初始化 UDP 组播连接 
    led_on(1);
    esp8266_connect_udp_multicast();
    led_off(1);

    // 3. 进入主循环，不断采集并发送数据
    while(1)
    {
        count++;
        // 每 5 秒钟读取并上传一次数据 (10 * 500ms = 5000ms)
        if(count % 10 == 0) {
            
            get_dht_value(dht_data);
            
            // 校验 DHT11 数据完整性
            if ((u8)(dht_data[0] + dht_data[1] + dht_data[2] + dht_data[3]) == dht_data[4])
            {
                current_hum = dht_data[0] + dht_data[1] * 0.1f;
                current_temp = dht_data[2] + dht_data[3] * 0.1f;
                
                // [修改] 分两次发送，一条温度，一条湿度。加100ms延时防止网卡粘包
                send_sensor_data(TEMP_SENSOR_ID, current_temp);
                delay_ms(100);
                send_sensor_data(HUM_SENSOR_ID, current_hum);
            }
            else
            {
                // 容错处理：单总线受干扰导致校验失败，发送上一次正确的温湿度
                send_sensor_data(TEMP_SENSOR_ID, current_temp);
                delay_ms(100);
                send_sensor_data(HUM_SENSOR_ID, current_hum);
            }
        }
        delay_ms(100);
    }
}