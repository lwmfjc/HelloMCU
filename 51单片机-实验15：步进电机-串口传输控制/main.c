/**************************************************************************************
* 实验现象：
*       下载程序后，步进电机自动旋转
*
*       串口发送：
*       1：改变电机旋转方向
*       2：电机加速
*       3：电机减速
*       0：运行/停止切换
*       s：显示当前电机状态
*
* 接线说明：
*       单片机 --> 四线双极性步进电机模块
*               P10 --> IA
*               P11 --> IB
*               P12 --> IC
*               P13 --> ID
*
*       串口：
*               使用开发板原有串口
*
***************************************************************************************/

#include "reg52.h"

typedef unsigned int  u16;
typedef unsigned char u8;


/*==================== 电机引脚 ====================*/

sbit MOTOA = P1^0;
sbit MOTOB = P1^1;
sbit MOTOC = P1^2;
sbit MOTOD = P1^3;


/*==================== 电机参数 ====================*/

/*
 * SPEED越小，电机越快
 * SPEED越大，电机越慢
 */
u16 SPEED = 200;


/*
 * direction = 1：正转
 * direction = 0：反转
 */
bit direction = 1;


/*
 * motor_run = 1：运行
 * motor_run = 0：停止
 */
bit motor_run = 1;


/*==================== 串口命令计数 ====================*/

/*
 * 不再只使用一个uart_command变量。
 *
 * 这样连续发送多个命令时，
 * 不容易因为电机正在delay而丢失命令。
 */

u8 cmd_1_count = 0;
u8 cmd_2_count = 0;
u8 cmd_3_count = 0;
u8 cmd_0_count = 0;
u8 cmd_s_count = 0;


/*******************************************************************************
* 函数名       : delay
* 函数功能     : 延时函数
*******************************************************************************/
void delay(u16 i)
{
    while(i--);
}


/*******************************************************************************
* 函数名       : motor_forward
* 函数功能     : 电机正转
*
* 注意：
*       这里完全保持原来能够正常转动的代码
*******************************************************************************/
void motor_forward()
{
    MOTOA = 1;
    MOTOB = 0;
    MOTOC = 1;
    MOTOD = 1;
    delay(SPEED);

    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 0;
    delay(SPEED);

    MOTOA = 0;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 1;
    delay(SPEED);

    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 0;
    MOTOD = 1;
    delay(SPEED);
}


/*******************************************************************************
* 函数名       : motor_reverse
* 函数功能     : 电机反转
*
* 注意：
*       这里完全保持原来能够正常转动的代码
*******************************************************************************/
void motor_reverse()
{
    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 0;
    MOTOD = 1;
    delay(SPEED);

    MOTOA = 0;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 1;
    delay(SPEED);

    MOTOA = 1;
    MOTOB = 1;
    MOTOC = 1;
    MOTOD = 0;
    delay(SPEED);

    MOTOA = 1;
    MOTOB = 0;
    MOTOC = 1;
    MOTOD = 1;
    delay(SPEED);
}


/*******************************************************************************
* 函数名       : motor_stop
* 函数功能     : 停止电机并关闭绕组
*******************************************************************************/
void motor_stop()
{
    MOTOA = 0;
    MOTOB = 0;
    MOTOC = 0;
    MOTOD = 0;
}


/*******************************************************************************
* 函数名       : uart_init
* 函数功能     : 串口初始化
*******************************************************************************/
void uart_init()
{
    /*
     * 定时器1方式2
     */
    TMOD &= 0x0F;
    TMOD |= 0x20;

    /*
     * 串口方式1
     * 允许接收
     */
    SCON = 0x50;

    /*
     * SMOD = 1
     */
    PCON = 0x80;

    /*
     * 11.0592MHz晶振
     * 9600波特率
     */
    TH1 = 0xFA;
    TL1 = 0xFA;

    /*
     * 清除串口标志
     */
    TI = 0;
    RI = 0;

    /*
     * 开启串口中断
     */
    ES = 1;

    /*
     * 开启总中断
     */
    EA = 1;

    /*
     * 启动定时器1
     */
    TR1 = 1;
}


/*******************************************************************************
* 函数名       : uart_send
* 函数功能     : 串口发送一个字符
*******************************************************************************/
void uart_send(u8 dat)
{
    /*
     * 发送的时候暂时关闭串口中断
     *
     * 防止TI和接收中断产生干扰
     */
    ES = 0;

    TI = 0;

    SBUF = dat;

    while(!TI);

    TI = 0;

    /*
     * 恢复串口接收中断
     */
    ES = 1;
}


/*******************************************************************************
* 函数名       : uart_send_string
* 函数功能     : 串口发送字符串
*******************************************************************************/
void uart_send_string(char *str)
{
    while(*str)
    {
        uart_send(*str);
        str++;
    }
}


/*******************************************************************************
* 函数名       : send_number
* 函数功能     : 串口发送数字
*******************************************************************************/
void send_number(u16 num)
{
    char buf[6];
    u8 i = 0;

    if(num == 0)
    {
        uart_send('0');
        return;
    }

    while(num > 0)
    {
        buf[i++] = num % 10 + '0';
        num /= 10;
    }

    while(i > 0)
    {
        uart_send(buf[--i]);
    }
}


/*******************************************************************************
* 函数名       : process_command
* 函数功能     : 处理串口命令
*******************************************************************************/
void process_command()
{
    /*
     *==================== 1：改变方向 ====================
     */

    if(cmd_1_count > 0)
    {
        /*
         * 每收到一个1，改变一次方向
         */
        if(direction == 1)
        {
            direction = 0;
        }
        else
        {
            direction = 1;
        }

        cmd_1_count--;

        uart_send_string("Direction: ");

        if(direction == 1)
        {
            uart_send_string("Forward\r\n");
        }
        else
        {
            uart_send_string("Reverse\r\n");
        }
    }


    /*
     *==================== 2：加速 ====================
     */

    if(cmd_2_count > 0)
    {
        if(SPEED >= 140)
        {
            SPEED = SPEED - 30;
        }

        cmd_2_count--;

        uart_send_string("Speed: ");
        send_number(SPEED);
        uart_send_string("\r\n");
    }


    /*
     *==================== 3：减速 ====================
     */

    if(cmd_3_count > 0)
    {
        if(SPEED <= 220)
        {
            SPEED = SPEED + 30;
        }

        cmd_3_count--;

        uart_send_string("Speed: ");
        send_number(SPEED);
        uart_send_string("\r\n");
    }


    /*
     *==================== 0：运行/停止 ====================
     */

    if(cmd_0_count > 0)
    {
        if(motor_run == 1)
        {
            motor_run = 0;

            /*
             * 停止以后立即关闭电机绕组
             */
            motor_stop();

            uart_send_string("Motor: STOP\r\n");
        }
        else
        {
            motor_run = 1;

            uart_send_string("Motor: RUN\r\n");
        }

        cmd_0_count--;
    }


    /*
     *==================== s：显示状态 ====================
     */

    if(cmd_s_count > 0)
    {
        cmd_s_count--;

        uart_send_string("===== MOTOR STATUS =====\r\n");


        uart_send_string("Motor: ");

        if(motor_run == 1)
        {
            uart_send_string("RUN\r\n");
        }
        else
        {
            uart_send_string("STOP\r\n");
        }


        uart_send_string("Direction: ");

        if(direction == 1)
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


/*******************************************************************************
* 函数名       : uart_isr
* 函数功能     : 串口中断服务函数
*******************************************************************************/
void uart_isr() interrupt 4
{
    u8 dat;


    /*
     * 只处理接收
     */
    if(RI)
    {
        RI = 0;

        /*
         * 读取收到的数据
         */
        dat = SBUF;


        /*
         * 收到1
         */
        if(dat == '1')
        {
            if(cmd_1_count < 250)
            {
                cmd_1_count++;
            }
        }


        /*
         * 收到2
         */
        else if(dat == '2')
        {
            if(cmd_2_count < 250)
            {
                cmd_2_count++;
            }
        }


        /*
         * 收到3
         */
        else if(dat == '3')
        {
            if(cmd_3_count < 250)
            {
                cmd_3_count++;
            }
        }


        /*
         * 收到0
         */
        else if(dat == '0')
        {
            if(cmd_0_count < 250)
            {
                cmd_0_count++;
            }
        }


        /*
         * 收到s或S
         */
        else if(dat == 's' || dat == 'S')
        {
            if(cmd_s_count < 250)
            {
                cmd_s_count++;
            }
        }


        /*
         * 回车和换行忽略
         */
    }
}


/*******************************************************************************
* 函数名       : main
* 函数功能     : 主函数
*******************************************************************************/
void main()
{
    /*
     * P1.0~P1.3用于控制步进电机
     */
    P1 = 0x00;


    /*
     * 初始化串口
     */
    uart_init();


    /*
     * 上电后默认运行
     */
    motor_run = 1;


    while(1)
    {
        /*
         * 处理串口命令
         */
        process_command();


        /*
         * 电机运行
         */
        if(motor_run == 1)
        {
            if(direction == 1)
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