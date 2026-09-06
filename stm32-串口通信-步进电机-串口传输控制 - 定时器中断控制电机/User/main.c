#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "motor.h"
#include "timer.h"

int main()
{
    SysTick_Init(72);

    NVIC_PriorityGroupConfig(
        NVIC_PriorityGroup_2
    );

    LED_Init();

    USART1_Init(9600);

    Motor_Init();

    /*
       初始化TIM4电机定时器
    */

    TIM4_Motor_Init();

    motor_run = 1;

    while(1)
    {
        /*
           主循环现在不负责产生
           电机步进脉冲。

           电机由TIM4中断负责。
        */

        process_command();


        /*
           这里以后可以继续增加其他任务：

           传感器读取
           编码器处理
           通信
           PID
           LED
           等等
        */
    }
}