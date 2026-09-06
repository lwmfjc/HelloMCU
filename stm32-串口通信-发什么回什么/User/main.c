/*******************************************************************************
* 实验名称  	: USART串口通信实验
* 实验说明  : 注意：打开串口调试助手后，要先勾选DTR后再取消，见实验截图
* 接线说明  : 				
* 实验现象	: 核心板上D1指示灯闪烁，打开串口调试助手，实验现象可见文件内截图
*******************************************************************************/

#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"

int main()
{
	u8 i=0;  
	SysTick_Init(72);
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //中断优先级分组 分2组
	LED_Init();
	USART1_Init(9600);
	
	while(1)
	{
		i++;
		if(i%20==0)
		{
			led1=!led1;
		}
		delay_ms(10);
	}
}
