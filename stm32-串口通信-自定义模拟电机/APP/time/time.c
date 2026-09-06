#include "time.h"
#include "usart.h"


/************************************************
 * 定时计数
 ************************************************/

volatile u16 timer_count = 0;


/************************************************
 * TIM4初始化
 *
 * STM32F103C8T6
 *
 * 假设系统时钟：
 *
 * SYSCLK = 72MHz
 *
 * APB1 = 36MHz
 *
 * 但是APB1定时器时钟自动×2
 *
 * TIM4CLK = 72MHz
 *
 *
 * PSC = 7199
 *
 * 72MHz / 7200
 * = 10kHz
 *
 *
 * ARR = 99
 *
 * 100 / 10000
 * = 10ms
 *
 *
 * 因此：
 *
 * TIM4每10ms产生一次更新中断
 *
 * 50次：
 *
 * 10ms × 50
 * = 500ms
 ************************************************/

void TIM4_Init(u16 per, u16 psc)
{
    TIM_TimeBaseInitTypeDef
        TIM_TimeBaseInitStructure;

    NVIC_InitTypeDef
        NVIC_InitStructure;


    /********************************************
     * 开启TIM4时钟
     ********************************************/

    RCC_APB1PeriphClockCmd(
        RCC_APB1Periph_TIM4,
        ENABLE
    );


    /********************************************
     * TIM4基本参数
     ********************************************/

    TIM_TimeBaseInitStructure.TIM_Period =
        per;


    TIM_TimeBaseInitStructure.TIM_Prescaler =
        psc;


    TIM_TimeBaseInitStructure.TIM_ClockDivision =
        TIM_CKD_DIV1;


    TIM_TimeBaseInitStructure.TIM_CounterMode =
        TIM_CounterMode_Up;


    TIM_TimeBaseInit(
        TIM4,
        &TIM_TimeBaseInitStructure
    );


    /********************************************
     * 开启更新中断
     ********************************************/

    TIM_ITConfig(
        TIM4,
        TIM_IT_Update,
        ENABLE
    );


    /********************************************
     * 清除中断标志
     ********************************************/

    TIM_ClearITPendingBit(
        TIM4,
        TIM_IT_Update
    );


    /********************************************
     * TIM4 NVIC配置
     ********************************************/

    NVIC_InitStructure.NVIC_IRQChannel =
        TIM4_IRQn;


    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        2;


    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        3;


    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;


    NVIC_Init(
        &NVIC_InitStructure
    );


    /********************************************
     * 启动TIM4
     ********************************************/

    TIM_Cmd(
        TIM4,
        ENABLE
    );
}


/************************************************
 * TIM4中断
 *
 * 每10ms进入一次
 *
 * 50次 = 500ms
 ************************************************/

void TIM4_IRQHandler(void)
{
    /********************************************
     * 检查TIM4更新中断
     ********************************************/

    if(
        TIM_GetITStatus(
            TIM4,
            TIM_IT_Update
        ) != RESET
    )
    {
        /****************************************
         * 定时计数
         ****************************************/

        timer_count++;


        /****************************************
         * 每500ms更新一次电机状态
         ****************************************/

        if(timer_count >= 50)
        {
            timer_count = 0;


            /************************************
             * 当前位于目标位置左侧
             *
             * 正方向运动
             ************************************/

            if(
                current_position <
                target_position
            )
            {
                /*
                 * 速度为0
                 *
                 * 电机不动
                 */

                if(target_velocity == 0)
                {
                    current_velocity = 0;
                }
                else
                {
                    /*
                     * 向正方向移动
                     */

                    current_position +=
                        target_velocity;


                    current_velocity =
                        target_velocity;


                    /*
                     * 防止超过目标位置
                     */

                    if(
                        current_position >=
                        target_position
                    )
                    {
                        current_position =
                            target_position;


                        current_velocity = 0;
                    }
                }
            }


            /************************************
             * 当前位于目标位置右侧
             *
             * 负方向运动
             ************************************/

            else if(
                current_position >
                target_position
            )
            {
                /*
                 * 速度为0
                 */

                if(target_velocity == 0)
                {
                    current_velocity = 0;
                }
                else
                {
                    /*
                     * 向负方向移动
                     */

                    current_position -=
                        target_velocity;


                    current_velocity =
                        -target_velocity;


                    /*
                     * 防止超过目标位置
                     */

                    if(
                        current_position <=
                        target_position
                    )
                    {
                        current_position =
                            target_position;


                        current_velocity = 0;
                    }
                }
            }


            /************************************
             * 已经到达目标位置
             ************************************/

            else
            {
                current_velocity = 0;
            }
        }


        /****************************************
         * 清除TIM4更新中断标志
         ****************************************/

        TIM_ClearITPendingBit(
            TIM4,
            TIM_IT_Update
        );
    }
}