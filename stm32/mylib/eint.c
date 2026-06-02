#include "eint.h"

eint_handler e_h0, e_h1, e_h2;//定义了3个全局的函数指针

void eint_init(void)//初始化功能按键使用的GPIO管脚和外部中断以及终端控制器
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量
	NVIC_InitTypeDef Nvic_Value;//定义了NVIC初始化结构体类型的变量
	EXTI_InitTypeDef Exti_Value;//定义了EXTI初始化结构体类型的变量
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
	//通过APB2总线使能GPIOC组和GPIOA组以及AFIO的时钟
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IPU;//选择了上拉输入的模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_8;//选择了 9 8 号管脚
	GPIO_Init(GPIOC, &Gpio_Value);//按照上述配置初始化GPIOC组的管脚
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IPD;//选择了下拉输入的模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_0;//选择了 0 号管脚
	GPIO_Init(GPIOA, &Gpio_Value);//按照上述配置初始化GPIOA组的管脚	

	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource9);//为EXTI9选择由PC9管脚触发
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOC, GPIO_PinSource8);//为EXTI8选择由PC8管脚触发
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOA, GPIO_PinSource0);//为EXTI0选择由PA0管脚触发

	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//选择抢占优先级和响应优先级的所占比例为2:2
	Nvic_Value.NVIC_IRQChannel = EXTI9_5_IRQn;//选择了EXTI9_5的中断号
	Nvic_Value.NVIC_IRQChannelCmd = ENABLE;//选择了使能该中断
	Nvic_Value.NVIC_IRQChannelPreemptionPriority = 2;//选择了抢占优先级的级别为2
	Nvic_Value.NVIC_IRQChannelSubPriority = 2;//选择了响应优先级的级别为2
	NVIC_Init(&Nvic_Value);//按照上述配置初始化NVIC
	
	Nvic_Value.NVIC_IRQChannel = EXTI0_IRQn;//选择了EXTI0的中断号
	NVIC_Init(&Nvic_Value);//按照上述配置初始化NVIC
	
	Exti_Value.EXTI_Line = EXTI_Line9 | EXTI_Line8;//选择了EXTI9 和 EXTI8
	Exti_Value.EXTI_LineCmd = ENABLE;//选择了使能该外部中断
	Exti_Value.EXTI_Mode = EXTI_Mode_Interrupt;//选择了中断的模式
	Exti_Value.EXTI_Trigger = EXTI_Trigger_Falling;//选择了下降沿触发
	EXTI_Init(&Exti_Value);//按照上述配置初始化EXTI9 和 EXTI8
	
	Exti_Value.EXTI_Line = EXTI_Line0;//选择了EXTI0
	Exti_Value.EXTI_Trigger = EXTI_Trigger_Rising;//选择了上升沿触发
	EXTI_Init(&Exti_Value);//按照上述配置初始化EXTI0
}

void EXTI9_5_IRQHandler(void)//EXTI9_5的中断处理函数
{
	if(EXTI_GetITStatus(EXTI_Line9) == SET)//判断是否是由EXTI9触发的中断
	{
		e_h0();//调用客户指定的函数功能
		EXTI_ClearITPendingBit(EXTI_Line9);//清除EXTI9的中断标志位
	}
	if(EXTI_GetITStatus(EXTI_Line8) == SET)//判断是否是由EXTI8触发的中断
	{
		e_h1();//调用客户指定的函数功能
		EXTI_ClearITPendingBit(EXTI_Line8);//清除EXTI8的中断标志位
	}
}

void EXTI0_IRQHandler(void)
{
	e_h2();//调用客户指定的函数功能
	EXTI_ClearITPendingBit(EXTI_Line0);//清除EXTI0的中断标志位
}

void set_eint_handler(eint_handler h0, eint_handler h1, eint_handler h2)//设置回调函数
{
	e_h0 = h0;//把形参h0存储的地址,转存到全局函数指针e_h0中
	e_h1 = h1;//把形参h1存储的地址,转存到全局函数指针e_h1中
	e_h2 = h2;//把形参h2存储的地址,转存到全局函数指针e_h2中
}
















