#include "system.h"
#include "SysTick.h"
#include "led.h"
#include "usart.h"
#include "motor.h"


int main()
{
    SysTick_Init(72);

    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    LED_Init();

    USART1_Init(9600);

    Motor_Init();


    /*
     * 和原51程序一样：
     *
     * 上电以后自动运行
     */
    motor_run = 1;


    while(1)
    {
        /*
         * 先处理串口命令
         */
        process_command();


        /*
         * 电机运行
         */
        if(motor_run)
        {
            if(direction)
            {
                motor_forward();
            }
            else
            {
                motor_reverse();
            }
        }

        /*
         * 电机停止
         */
        else
        {
            motor_stop();
        }
    }
}