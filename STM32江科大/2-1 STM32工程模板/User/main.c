#include "stm32f10x.h" //定义STM32F103芯片里面所有外设的地址和结构

int main(void) {
    //1.使能GPIOC的时钟
    //GPIOC都是RCC_APB2的外设
    //设为1则是打开GPIOC的时钟
	//打开GPIOC的使用
	//RCC->APB2ENR=0x00000010;
    RCC->APB2ENR |= RCC_APB2ENR_IOPAEN;
	
	//2.配置PC 13口的模式
	//GPIOC->CRH=0x00300000;
	GPIOA->CRL &= 0xFFFFFFF0;
    GPIOA->CRL |= 0x00000003;
	
	
	//3. 给PC 13口输出数据
	//由于灯是低电平点亮的，所以给全0就是灭，全1就是暗
	//GPIOC->ODR=0x00002000;
	//GPIOA->ODR |= (1<<0);//灯亮
	GPIOA->ODR &= ~(1 << 0);//灯灭


	
    while (1) {


    }
} 
