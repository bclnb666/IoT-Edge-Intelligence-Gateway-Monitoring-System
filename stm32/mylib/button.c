#include "button.h"
#include "bitband.h"

void button_init(void)//初始化功能按键使用的GPIO管脚
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA, ENABLE);
	//通过APB2总线使能GPIOC组和GPIOA组的时钟
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IPU;//选择了上拉输入的模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;//选择了 9 8 号管脚
	GPIO_Init(GPIOC, &Gpio_Value);//按照上述配置初始化GPIOC组的管脚
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IPD;//选择了下拉输入的模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_0;//选择了 0 号管脚
	GPIO_Init(GPIOA, &Gpio_Value);//按照上述配置初始化GPIOA组的管脚	
}

int button_status(int num)//根据形参检测相应按键的电平高低
{
	int ret = 0;//用来存储获取到的电平高低
	
	switch(num)
	{
		case 0 : ret = PCIn(9); break;//获取PC9管脚的电平高低
		case 1 : ret = PCIn(8); break;//获取PC8管脚的电平高低
		case 2 : ret = PAIn(0); ret = !ret; break;//获取PA0管脚的电平高低
	}
	
	return !ret;
}













