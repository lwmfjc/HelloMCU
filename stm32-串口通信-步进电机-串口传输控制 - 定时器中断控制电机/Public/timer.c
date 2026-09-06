#include "timer.h"
#include "motor.h"

void TIM4_Motor_Init(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启 TIM4 时钟 */
    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM4,
        ENABLE
    );

    /*
       STM32F103：

       假设 TIM4 时钟 = 72MHz

       我们设置：

       PSC = 71

       72MHz / (71 + 1)
       = 1MHz

       也就是：

       1个计数 = 1us
    */

    TIM_TimeBaseStructure.TIM_Prescaler = 71;

    /*
       ARR = SPEED - 1

       SPEED=2000

       2000us产生一次中断
    */

    TIM_TimeBaseStructure.TIM_Period = SPEED - 1;

    TIM_TimeBaseStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;

    TIM_TimeBaseStructure.TIM_CounterMode =
        TIM_CounterMode_Up;

    TIM_TimeBaseInit(
        TIM4,
        &TIM_TimeBaseStructure
    );


    /* 允许更新中断 */

    TIM_ITConfig(
        TIM4,
        TIM_IT_Update,
        ENABLE
    );


    /* 配置NVIC */

    NVIC_InitStructure.NVIC_IRQChannel =
        TIM4_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        2;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        3;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;

    NVIC_Init(&NVIC_InitStructure);


    /* 启动TIM4 */

    TIM_Cmd(TIM4, ENABLE);
}


/* =========================
   TIM4中断
   ========================= */

void TIM4_IRQHandler(void)
{
    if(TIM_GetITStatus(
        TIM4,
        TIM_IT_Update
    ) != RESET)
    {
        /* 清除中断标志 */

        TIM_ClearITPendingBit(
            TIM4,
            TIM_IT_Update
        );


        /*
           电机运行时：
           每次中断走一步
        */

        if(motor_run)
        {
            motor_step();
        }
        else
        {
            motor_stop();
        }


        /*
           SPEED可能被串口命令修改，
           所以实时更新ARR
        */

        TIM_SetAutoreload(
            TIM4,
            SPEED - 1
        );
    }
}