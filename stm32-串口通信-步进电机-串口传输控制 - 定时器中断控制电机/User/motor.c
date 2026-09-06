#include "motor.h"
#include "usart.h"

/* =========================
   电机参数
   ========================= */

u16 SPEED = 2000;

u8 direction = 1;
u8 motor_run = 1;

/* =========================
   串口命令计数
   ========================= */

volatile u8 cmd_1_count = 0;
volatile u8 cmd_2_count = 0;
volatile u8 cmd_3_count = 0;
volatile u8 cmd_0_count = 0;
volatile u8 cmd_s_count = 0;

/* =========================
   当前步进位置
   0 ~ 3
   ========================= */

static u8 motor_step_index = 0;


/* =========================
   电机初始化
   ========================= */

void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_0 |
        GPIO_Pin_1 |
        GPIO_Pin_2 |
        GPIO_Pin_3;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    motor_stop();

    motor_step_index = 0;
}


/* =========================
   停止电机
   ========================= */

void motor_stop(void)
{
    MOTOA = 0;
    MOTOB = 0;
    MOTOC = 0;
    MOTOD = 0;
}


/* =========================
   执行一步
   ========================= */

void motor_step(void)
{
    /*
       正转：

       1011
       1110
       0111
       1101
    */

    if(direction)
    {
        switch(motor_step_index)
        {
            case 0:
                MOTOA = 1;
                MOTOB = 0;
                MOTOC = 1;
                MOTOD = 1;
                break;

            case 1:
                MOTOA = 1;
                MOTOB = 1;
                MOTOC = 1;
                MOTOD = 0;
                break;

            case 2:
                MOTOA = 0;
                MOTOB = 1;
                MOTOC = 1;
                MOTOD = 1;
                break;

            case 3:
                MOTOA = 1;
                MOTOB = 1;
                MOTOC = 0;
                MOTOD = 1;
                break;
        }

        motor_step_index++;

        if(motor_step_index >= 4)
            motor_step_index = 0;
    }

    /*
       反转：

       1101
       0111
       1110
       1011
    */

    else
    {
        switch(motor_step_index)
        {
            case 0:
                MOTOA = 1;
                MOTOB = 1;
                MOTOC = 0;
                MOTOD = 1;
                break;

            case 1:
                MOTOA = 0;
                MOTOB = 1;
                MOTOC = 1;
                MOTOD = 1;
                break;

            case 2:
                MOTOA = 1;
                MOTOB = 1;
                MOTOC = 1;
                MOTOD = 0;
                break;

            case 3:
                MOTOA = 1;
                MOTOB = 0;
                MOTOC = 1;
                MOTOD = 1;
                break;
        }

        motor_step_index++;

        if(motor_step_index >= 4)
            motor_step_index = 0;
    }
}


/* =========================
   串口发送字符串
   ========================= */

static void uart_send_string(char *str)
{
    while(*str)
    {
        USART_SendData(USART1, *str);

        while(USART_GetFlagStatus(
            USART1,
            USART_FLAG_TC
        ) != SET);

        str++;
    }
}


/* =========================
   发送数字
   ========================= */

static void send_number(u16 num)
{
    char buf[6];
    u8 i = 0;

    if(num == 0)
    {
        uart_send_string("0");
        return;
    }

    while(num > 0)
    {
        buf[i++] = (num % 10) + '0';
        num /= 10;
    }

    while(i > 0)
    {
        i--;
        USART_SendData(USART1, buf[i]);

        while(USART_GetFlagStatus(
            USART1,
            USART_FLAG_TC
        ) != SET);
    }
}


/* =========================
   处理串口命令
   ========================= */

void process_command(void)
{
    /* 1：改变方向 */

    if(cmd_1_count > 0)
    {
        direction = !direction;

        cmd_1_count--;

        if(direction)
        {
            uart_send_string(
                "Direction: Forward\r\n"
            );
        }
        else
        {
            uart_send_string(
                "Direction: Reverse\r\n"
            );
        }
    }


    /* 2：加速 */

    if(cmd_2_count > 0)
    {
        if(SPEED > SPEED_MIN)
        {
            if(SPEED - SPEED_STEP < SPEED_MIN)
                SPEED = SPEED_MIN;
            else
                SPEED -= SPEED_STEP;
        }

        cmd_2_count--;

        uart_send_string("Speed: ");
        send_number(SPEED);
        uart_send_string("\r\n");
    }


    /* 3：减速 */

    if(cmd_3_count > 0)
    {
        if(SPEED < SPEED_MAX)
        {
            if(SPEED + SPEED_STEP > SPEED_MAX)
                SPEED = SPEED_MAX;
            else
                SPEED += SPEED_STEP;
        }

        cmd_3_count--;

        uart_send_string("Speed: ");
        send_number(SPEED);
        uart_send_string("\r\n");
    }


    /* 0：停止/运行 */

    if(cmd_0_count > 0)
    {
        motor_run = !motor_run;

        cmd_0_count--;

        if(motor_run)
        {
            uart_send_string(
                "Motor: RUN\r\n"
            );
        }
        else
        {
            motor_stop();

            uart_send_string(
                "Motor: STOP\r\n"
            );
        }
    }


    /* s：显示状态 */

    if(cmd_s_count > 0)
    {
        cmd_s_count--;

        uart_send_string(
            "===== MOTOR STATUS =====\r\n"
        );

        if(motor_run)
        {
            uart_send_string(
                "Motor: RUN\r\n"
            );
        }
        else
        {
            uart_send_string(
                "Motor: STOP\r\n"
            );
        }

        if(direction)
        {
            uart_send_string(
                "Direction: Forward\r\n"
            );
        }
        else
        {
            uart_send_string(
                "Direction: Reverse\r\n"
            );
        }

        uart_send_string("SPEED: ");
        send_number(SPEED);
        uart_send_string("\r\n");

        uart_send_string(
            "========================\r\n"
        );
    }
}