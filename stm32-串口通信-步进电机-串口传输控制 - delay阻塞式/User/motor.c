#include "motor.h"
#include "usart.h"

#define SPEED_MIN 1000
#define SPEED_MAX 5000
#define SPEED_STEP 200

/*
 * 电机速度参数
 *
 * 和原51程序保持一致：
 * SPEED越小 -> 越快
 * SPEED越大 -> 越慢
 */
u16 SPEED = 2000;

/*
 * 电机方向
 *
 * 1 = 正转
 * 0 = 反转
 */
u8 direction = 1;

/*
 * 电机运行状态
 *
 * 1 = 运行
 * 0 = 停止
 */
u8 motor_run = 1;


/*
 * 命令计数器
 *
 * 和原来的51程序保持一致。
 *
 * 为什么不用一个 command 变量？
 *
 * 因为电机运行过程中存在 delay_us()，
 * 如果连续发送：
 *
 * 1
 * 2
 * 3
 *
 * 三个命令都可以暂存在这里，
 * 不会因为电机正在转而丢失。
 */
volatile u8 cmd_1_count = 0;
volatile u8 cmd_2_count = 0;
volatile u8 cmd_3_count = 0;
volatile u8 cmd_0_count = 0;
volatile u8 cmd_s_count = 0;


/*
 * 初始化步进电机GPIO
 */
void Motor_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    /*
     * MOTOA~MOTOD目前使用GPIOA
     */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 |
                                  GPIO_Pin_1 |
                                  GPIO_Pin_2 |
                                  GPIO_Pin_3;

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;

    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /*
     * 初始全部关闭
     */
    motor_stop();
}


/*
 * 停止电机
 *
 * 原51：
 *
 * P1 = 0;
 *
 * 即 IA IB IC ID 全部为0
 */
void motor_stop(void)
{
    MOTOA = 0;
    MOTOB = 0;
    MOTOC = 0;
    MOTOD = 0;
}


/*
 * 正转一步
 *
 * 原51程序：
 *
 * 1011
 * 1110
 * 0111
 * 1101
 *
 * 每一步之间 delay(SPEED)
 */
void motor_forward(void)
{
    MOTOA = 1;
    MOTOB = 0;
    MOTOC = 1;
    MOTOD = 1;

    delay_us(SPEED);


    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 0;

    delay_us(SPEED);


    MOTOA = 0;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 1;

    delay_us(SPEED);


    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 0;
    MOTOD = 1;

    delay_us(SPEED);
}


/*
 * 反转一步
 *
 * 原51程序：
 *
 * 1101
 * 0111
 * 1110
 * 1011
 */
void motor_reverse(void)
{
    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 0;
    MOTOD = 1;

    delay_us(SPEED);


    MOTOA = 0;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 1;

    delay_us(SPEED);


    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 0;

    delay_us(SPEED);


    MOTOA = 1;
    MOTOB = 0;
    MOTOC = 1;
    MOTOD = 1;

    delay_us(SPEED);
}


/*
 * 发送字符串
 */
static void uart_send_string(char *str)
{
    while(*str)
    {
        USART_SendData(USART1, *str++);

        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    }
}


/*
 * 发送一个数字
 *
 * 和原来的 send_number() 对应
 */
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
        USART_SendData(USART1, buf[--i]);

        while(USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET);
    }
}


/*
 * 处理串口命令
 *
 * 完全对应原51程序 process_command()
 */
void process_command(void)
{
    /*
     * ==========================================
     * 命令 1：切换方向
     * ==========================================
     */
    if(cmd_1_count > 0)
    {
        direction = !direction;

        cmd_1_count--;

        if(direction)
        {
            uart_send_string("Direction: Forward\r\n");
        }
        else
        {
            uart_send_string("Direction: Reverse\r\n");
        }
    }


    /*
     * ==========================================
     * 命令 2：加速
     *
     * SPEED越小越快
     * ==========================================
     */
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


    /*
     * ==========================================
     * 命令 3：减速
     * ==========================================
     */
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


    /*
     * ==========================================
     * 命令 0：运行 / 停止
     * ==========================================
     */
    if(cmd_0_count > 0)
    {
        motor_run = !motor_run;

        cmd_0_count--;

        if(motor_run)
        {
            uart_send_string("Motor: RUN\r\n");
        }
        else
        {
            motor_stop();

            uart_send_string("Motor: STOP\r\n");
        }
    }


    /*
     * ==========================================
     * 命令 S：显示状态
     * ==========================================
     */
    if(cmd_s_count > 0)
    {
        cmd_s_count--;

        uart_send_string("===== MOTOR STATUS =====\r\n");

        uart_send_string("Motor: ");

        if(motor_run)
        {
            uart_send_string("RUN\r\n");
        }
        else
        {
            uart_send_string("STOP\r\n");
        }

        uart_send_string("Direction: ");

        if(direction)
        {
            uart_send_string("Forward\r\n");
        }
        else
        {
            uart_send_string("Reverse\r\n");
        }

        uart_send_string("SPEED: ");
        send_number(SPEED);
        uart_send_string("\r\n");

        uart_send_string("========================\r\n");
    }
}