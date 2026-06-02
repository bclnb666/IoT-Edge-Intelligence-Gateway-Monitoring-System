#include "iwdg.h"

void iwdg_init(int ms)//初始化IWDG
{
	IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);//打开PR寄存器和RLR寄存器的写访问(关闭写保护)
	IWDG_SetPrescaler(IWDG_Prescaler_64);//选择了64分频LSI(40KHz)/64 = 625Hz
	IWDG_SetReload(ms);//设置重装载值(计数值)
	IWDG_ReloadCounter();//喂狗的操作
	IWDG_Enable();//使能看门狗
}

void iwdg_feed_dog(void)//喂狗
{
	IWDG_ReloadCounter();//喂狗的操作
}






