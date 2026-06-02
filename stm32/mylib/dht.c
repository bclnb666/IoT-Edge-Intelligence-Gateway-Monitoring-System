#include "dht.h"
#include "bitband.h"
#include "delay.h"

static void set_dht_gpio_output(void)//把DHT11使用的GPIO管脚初始化为推挽输出
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_Out_PP;//选择了推挽的输出模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_10;//选择了 10 号管脚
	Gpio_Value.GPIO_Speed = GPIO_Speed_50MHz;//选择了50MHz的输出速率
	GPIO_Init(GPIOC, &Gpio_Value);//按照上述配置初始化GPIOC组的管脚
}

static void set_dht_gpio_input(void)//把DHT11使用的GPIO管脚初始化为浮空输入
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IN_FLOATING;//选择了浮空的输入模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_10;//选择了 10 号管脚
	GPIO_Init(GPIOC, &Gpio_Value);//按照上述配置初始化GPIOC组的管脚
}

static void dht_output_status(int state)//控制数据线输出高低电平
{
	set_dht_gpio_output();//把PC10管脚初始化为推挽的输出模式
	if(state == 0)//判断是否拉低数据线
		PCOut(10) = 0;
	else
		PCOut(10) = 1;
}

static int dht_input_status(void)//获取数据线的电平高低
{
	set_dht_gpio_input();//把PC10管脚初始化为浮空的输入模式
	return PCIn(10);
}

void dht_init(void)//初始化DHT11
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//通过APB2总线使能GPIOC组的时钟
}

void get_dht_value(u8 data[5])//获取DHT11采集的数据
{
	u8 flag = 0;//用来获取数据线的电平高低
	u8 time = 0;//计数变量
	u8 i = 0;//循环变量
	u8 tmp = 0;//中间变量
	
	dht_output_status(1);//拉高数据线
	dht_output_status(0);//拉低数据线
	delay_ms(20);//延时20ms
	dht_output_status(1);//拉高数据线
	//while(dht_input_status());//以死等的方式等待数据线被拉低
	do
	{
		flag = dht_input_status();//读取数据线的电平高低
		delay_us(2);//延时2us
		time++;//计数变量自增
	}while(flag == 1 && time <= 20);//以超时处理的方式等待数据线被拉低
	if(time > 20)//判断是否超时
		return ;//由于超时,结束函数
	while(!dht_input_status());//以死等的方式等待数据线被拉高
	//循环接收40bit数据,别忘了高位先出
	for(i = 0; i < 40; i++)//循环40次获取40bit的数据
	{
		while(dht_input_status());//以死等的方式等待数据线被拉低
		while(!dht_input_status());//以死等的方式等待数据线被拉高
		delay_us(40);//延时40us
		tmp <<= 1;
		if(dht_input_status())//读取数据线的高低电平
			tmp |= 1;
		if((i + 1) % 8 == 0)//i == 7 15 23 31 39
		{
			data[i / 8] = tmp;//把获取到的1byte数据,转存到形参数组中
			tmp = 0;//清空中间变量
		}
	}
	dht_output_status(1);//拉高数据线
}










