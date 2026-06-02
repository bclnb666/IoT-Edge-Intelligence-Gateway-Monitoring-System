#include "buzzer.h"
#include "bitband.h"

void buzzer_init(void)//初始化蜂鸣器使用的GPIO管脚
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	//通过APB2总线使能GPIOC组的时钟
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_Out_PP;//选择了推挽的输出模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_7;//选择了 7 号管脚
	Gpio_Value.GPIO_Speed = GPIO_Speed_50MHz;//选择了50MHz的输出速率
	GPIO_Init(GPIOC, &Gpio_Value);//按照上述配置初始化GPIOC组的管脚
	
	PCOut(7) = 0;//让PC7管脚输出低电平
}

void buzzer_on(void)//打开蜂鸣器
{
	PCOut(7) = 1;//让PC7管脚输出低电平
}

void buzzer_off(void)//关闭蜂鸣器
{
	PCOut(7) = 0;//让PC7管脚输出低电平
}



