	#include "stm32f10x.h" //包含STM32F103芯片的寄存器定义、外设地址和相关结构体


	int main(void)
	{
		//1. 开启GPIOA时钟
		//GPIOA属于APB2总线上的外设
		//STM32外设默认关闭时钟，需要先开启对应时钟才能使用
		//
		//RCC_APB2ENR寄存器负责控制APB2外设时钟
		//将IOPAEN位置1，即开启GPIOA时钟
		//
		//RCC->APB2ENR |= RCC_APB2ENR_IOPAEN; //开启GPIOA时钟
		RCC->APB2ENR = 0x00000004;  //直接操作寄存器方式
		


		//2. 配置PA0引脚模式
		//
		//GPIO的配置寄存器：
		//CRL负责配置PA0~PA7
		//CRH负责配置PA8~PA15
		//
		//每个GPIO引脚占用4个bit配置
		//PA0对应CRL最低4位 
		
		//设置PA0为推挽输出模式
		//MODE=11：输出速度50MHz
		//CNF=00：通用推挽输出
		//
		//最终配置：
		//PA0 = 输出模式，最大速度50MHz
		//复位或未配置时，GPIO 寄存器 ODR
		//（输出数据寄存器）默认值为 0。一旦将引
		//脚切为推挽输出模式，引脚硬件上会立即根据 ODR 的当
		//前值（0）驱动电平，因此 PA0 输出低电平。
		GPIOA->CRL |= 0x00000003;


		//3. 控制PA0输出电平
		//
		//普中开发板LED连接到PA0
		//
		//GPIO输出：
		//ODR对应GPIO输出数据寄存器
		//第0位对应PA0
		//
		//输出1：
		//PA0为高电平
		//
		//输出0：
		//PA0为低电平
		//
		//根据你的普中开发板硬件连接：
		//PA0输出低电平时LED熄灭
		//PA0输出高电平时LED点亮
		//
		//关闭LED
	    GPIOA->ODR &= ~(1 << 0);
	   //GPIOA->ODR |= (1<<0);//灯亮


		while (1)
		{

		}
	}
