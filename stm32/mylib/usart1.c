#include "usart1.h"
#include "stdio.h"

usart1_handler u1_h;//定义了全局的函数指针

//禁用半主机模式(一旦禁用了半主机模式,那么就找不到stdin和stdout)
#pragma import(__use_no_semihosting)

struct __FILE//弱定义了FILE类型
{
	int a;
};

FILE __stdout;//弱定义标准输出
FILE __stdin;//弱定义标准输入

void _sys_exit(void)//加上这个函数的实现,否则编译不过
{
	
}

int fputc(int c, FILE *fp)//使用usart1重新实现fputc(3)
{
	usart_1_send_byte(c);//调用USART1发送1byte数据
	return c;
}

void usart_1_init(void)//初始化USART1使用的管脚
{
	GPIO_InitTypeDef Gpio_Value;//定义了GPIO初始化结构体类型的变量
	USART_InitTypeDef Usart_Value;//定义了USART初始化结构体类型的变量
	NVIC_InitTypeDef Nvic_Value;//定义了NVIC初始化结构体类型的变量
	
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_USART1, ENABLE);
	//通过APB2总线使能GPIOA组以及USART1的时钟
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_AF_PP;//选择了推挽的复用模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_9;//选择了 9 号管脚
	Gpio_Value.GPIO_Speed = GPIO_Speed_50MHz;//选择了50MHz的输出速率
	GPIO_Init(GPIOA, &Gpio_Value);//按照上述配置初始化GPIOA组的管脚
	
	Gpio_Value.GPIO_Mode = GPIO_Mode_IN_FLOATING;//选择了浮空的输入模式
	Gpio_Value.GPIO_Pin = GPIO_Pin_10;//选择了 10 号管脚
	GPIO_Init(GPIOA, &Gpio_Value);//按照上述配置初始化GPIOA组的管脚
	
	Usart_Value.USART_BaudRate = 115200;//选择了115200的波特率
	Usart_Value.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//选择了没有硬件流控
	Usart_Value.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;//选择了发送和接收模式
	Usart_Value.USART_Parity = USART_Parity_No;//选择了没有奇偶校验
	Usart_Value.USART_StopBits = USART_StopBits_1;//选择了1个停止位
	Usart_Value.USART_WordLength = USART_WordLength_8b;//选择了8个数据位
	USART_Init(USART1, &Usart_Value);//按照上述配置初始化USART1
	
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//配置组优先级和子优先级的所占比例2:2
	Nvic_Value.NVIC_IRQChannel = USART1_IRQn;//选择了USART1的中断号
	Nvic_Value.NVIC_IRQChannelCmd = ENABLE;//选择了使能该中断
	Nvic_Value.NVIC_IRQChannelPreemptionPriority = 2;//选择了组优先级的级别为2
	Nvic_Value.NVIC_IRQChannelSubPriority = 2;//选择了子优先级的级别为2
	NVIC_Init(&Nvic_Value);//按照上述配置初始化NVIC
	
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//使能了USART1接收数据的中断
	
	USART_Cmd(USART1, ENABLE);//使能USART1的功能
}

void usart_1_send_byte(u8 data)//通过USART1发送1byte数据
{
	USART1->SR;//防止第一个字节数据丢失
	USART_SendData(USART1, data);//把形参data通过USART1进行发送
	while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);//以死等的方式等待USART1发送数据成功
	USART_ClearFlag(USART1, USART_FLAG_TC);//清除USART1发送数据成功的标志状态
}

void usart_1_send_data(u8 *buf)//通过USART1发送多个字节数据
{
	while(*buf)
	{
		usart_1_send_byte(*buf);
		buf++;
	}
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1, USART_IT_RXNE) == SET)//判断是否是由USART1的接收数据触发的中断
	{
		u1_h(USART_ReceiveData(USART1));//获取USART1接收到的数据
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);//清除USART1接收数据的中断标志位
	}
}

u8 usart_1_recv_byte(void)//通过USART1接收1byte数据
{
	u8 data = 0;//data变量存储接收到的数据
	
	if(USART_GetFlagStatus(USART1, USART_FLAG_RXNE) == SET)//判断USART1是否接收到了数据
	{
		data = USART_ReceiveData(USART1);//获取USART1接收到的数据
		USART_ClearFlag(USART1, USART_FLAG_RXNE);//清除USART1接收数据的标志状态
	}
	
	return data;//把接收到的USART1的数据返回
}

void set_usart1_handler(usart1_handler h)//设置回调函数
{
	u1_h = h;//把形参存储的函数地址转存到全局函数指针中
}






