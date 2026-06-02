#include "rtc.h"
#include "string.h"

const uint8_t monthTable[12] = {31,28,31,30,31,30,31,31,30,31,30,31};//用于存储每个月的天数(2月份需要+1)
static struct time_st T;//存储每一次获取的年月日星期时分秒

static uint8_t isLeapYear(uint16_t y)   
{
	return (((!(y % 4)) && (y % 100)) || (!(y % 400)));
}

void RTC_IRQHandler(void)
{
	if(RTC_GetITStatus(RTC_IT_SEC) == SET)//判断是否是由RTC的秒中断触发
	{
		conv_time(&T);//通过conv_time()把时间戳转换成时间存储到全局变量中
		RTC_ClearITPendingBit(RTC_IT_SEC);//清除RTC秒中断的中断标志位
	}
}

void rtc_config(struct time_st *t)//第一次操作RTC需要做的初始化
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE);
	//通过APB1总线把PWR和BKP的时钟使能
	//因为需要通过PWR外设使能BKP和RTC的写访问,所以需要打开PWR的时钟
	//因为需要通过BKP外设给备份寄存器写入特定的数值,将来判断是否是第一次操作RTC
	PWR_BackupAccessCmd(ENABLE);//开启备份区域和RTC的写访问(关闭写保护)
	BKP_DeInit();//复位BKP备份寄存器(因为之前可能使用过备份寄存器,所以复位一下)
	RCC_LSEConfig(RCC_LSE_ON);//通过RCC开启LSE(因为是第一次使用RTC,LSE可能没有开启过,所以开启一下)
	while(RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);//以死等的方式等待LSE稳定(因为时钟源刚开启并不稳定,不能直接使用,需要等待稳定)
	RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);//通过RCC为RTC选择由LSE提供时钟频率
	RCC_RTCCLKCmd(ENABLE);//通过RCC使能RTC的时钟
	RTC_WaitForSynchro();//等待RTC的时钟同步
	RTC_WaitForLastTask();//等待RTC最后一次写操作的完成
	RTC_ITConfig(RTC_IT_SEC, ENABLE);//使能RTC的秒中断
	RTC_WaitForLastTask();//等待RTC最后一次写操作的完成	
	RTC_EnterConfigMode();//进入RTC的配置模式
	RTC_SetPrescaler(32767);//设置RTC的预分频值(硬件上会默认+1<为了防止有的程序员在这里写0>)
	RTC_WaitForLastTask();//等待RTC最后一次写操作的完成	
	set_time(t);//把形参的时间转换成时间戳设置为RTC的计数值
	RTC_WaitForLastTask();//等待RTC最后一次写操作的完成	
	RTC_ExitConfigMode();//退出RTC的配置模式
}

void rtc_init(struct time_st *t)//初始化RTC的函数
{
	NVIC_InitTypeDef Nvic_Value;//定义了NVIC初始化结构体类型的变量
	/*
	[1]配置NVIC
	由于需要RTC每一秒钟计数器需要自增1,所以需要使用秒中断
	(无论是否是第一次使用RTC,都需要使能秒中断)
	*/
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//配置组优先级和子优先级的所占比例
	Nvic_Value.NVIC_IRQChannel = RTC_IRQn;//选择了RTC的中断号
	Nvic_Value.NVIC_IRQChannelCmd = ENABLE;//选择了使能该中断
	Nvic_Value.NVIC_IRQChannelPreemptionPriority = 2;//选择了组优先级的级别为2
	Nvic_Value.NVIC_IRQChannelSubPriority = 2;//选择了子优先级的级别为2
	NVIC_Init(&Nvic_Value);//按照上述配置初始化NVIC
	/*
	[2]判断是否是第一次使用RTC
	因为第一次使用RTC需要配置RTC的时钟频率以及设置计数值
	如果不是第一次使用RTC不需要操作这些
	*/
	if(BKP_ReadBackupRegister(BKP_DR6) != 0x9527)//判断是否是第一次操作RTC
	{
		rtc_config(t);//第一次操作RTC时要做的各种初始化
		BKP_WriteBackupRegister(BKP_DR6, 0x9527);//给3号备份寄存器写入特定的值
	}
	else//如果不是第一次操作RTC
	{
		RTC_WaitForSynchro();//等待RTC的时钟同步
		RTC_WaitForLastTask();//等待RTC最后一次写操作的完成
		RTC_ITConfig(RTC_IT_SEC, ENABLE);//使能RTC的秒中断
		RTC_WaitForLastTask();//等待RTC最后一次写操作的完成
	}
}

void set_time(struct time_st* t)
{
	uint32_t secCount = 0;
	uint16_t i;

	if(t->Y < 1970 || t->Y > 2099)
		return ;
	
	for(i = 1970; i < t->Y; i++)
	{
		if(isLeapYear(i))
			secCount += 31622400;
		else
			secCount += 31536000;
	}

	for (i = 0; i < t->M - 1; i++)
	{
		secCount += ((uint32_t)monthTable[i] * 86400);
		if (isLeapYear(t->Y) && i == 1)
			secCount += 86400;
	}
	secCount += (uint32_t)(t->D - 1) * 86400;    
	secCount += (uint32_t)(t->h) * 3600;
	secCount += (uint32_t)(t->m) * 60;
	secCount += (uint32_t)(t->s);

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR|RCC_APB1Periph_BKP, ENABLE);
	//通过APB1总线使能PWR和BKP的时钟
	PWR_BackupAccessCmd(ENABLE);//通过PWR使能备份区域的写访问(关闭写保护)
	RTC_SetCounter(secCount);//为RTC设置计数值
	RTC_WaitForLastTask();//等待RTC最后一次写操作的完成
}

void conv_time(struct time_st* t)
{
	uint32_t timeCount = 0;
	uint32_t Count = 0;
	uint16_t tmp = 0;
	
	timeCount = RTC_GetCounter();//获取RTC的计数值(时间戳)
	Count = timeCount / 86400;
	t->W = (4 + Count) % 7;
	if(Count != 0)
	{
		tmp = 1970;
		while(Count >= 365)
		{
			if(isLeapYear(tmp))
			{
				if(Count>= 366)
				{
					Count -= 366;
				}
				else
				{
					tmp++;
					break;
				}
			}
			else
			{
					Count -= 365;
			}
			tmp++;
		}
		t->Y = tmp;
		tmp = 0;
		while(Count >= 28)
		{
			if(isLeapYear(t->Y) && tmp == 1)
			{
				if(Count >= 29) 
						Count -= 29;
				else    
						break;
			}
			else
			{
				if(Count >= monthTable[tmp]) 
					Count -= monthTable[tmp];
				else
					break;
			}
			tmp++;
		}
		t->M = tmp + 1;
		t->D = Count + 1;
    }
    Count = timeCount % 86400;
    t->h = Count / 3600;
    t->m = (Count % 3600) / 60;
    t->s = (Count % 3600) % 60;
}

void get_time(struct time_st *t)//获取时间
{
	memcpy(t, &T, sizeof(T));//把全局变量中T的数据存储到形参指向的存储空间
}







