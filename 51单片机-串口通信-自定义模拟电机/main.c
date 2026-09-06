#include "REG52.h"
#include <string.h>

typedef unsigned int  u16;
typedef unsigned char u8;


/************************************************
 * 电机状态
 ************************************************/

int target_position = 0;     // 目标位置
int current_position = 0;    // 当前位置

int target_velocity = 1;     // 目标速度
int current_velocity = 0;    // 当前实际速度


/************************************************
 * 串口接收缓存
 ************************************************/

#define RX_BUFFER_SIZE 30

char rx_buffer[RX_BUFFER_SIZE];

u8 rx_index = 0;

bit command_ready = 0;

bit rx_overflow = 0;


/************************************************
 * 定时器
 ************************************************/

u16 timer_count = 0;


/************************************************
 * 串口初始化
 *
 * 9600 8N1
 ************************************************/

void uart_init()
{
    /*
        Timer1 模式2
        用于产生串口波特率
    */

    TMOD |= 0x20;


    /*
        串口模式1
        REN = 1，允许接收
    */

    SCON = 0x50;


    /*
        SMOD = 1
        波特率加倍
    */

    PCON = 0x80;


    /*
        11.0592MHz 晶振
        9600 波特率
    */

    TH1 = 0xFA;
    TL1 = 0xFA;


    /*
        开启串口中断
    */

    ES = 1;


    /*
        开启总中断
    */

    EA = 1;


    /*
        启动 Timer1
    */

    TR1 = 1;
}


/************************************************
 * Timer0 初始化
 *
 * 用于模拟电机运动
 ************************************************/

void timer0_init()
{
    /*
        清除 Timer0 模式位
    */

    TMOD &= 0xF0;


    /*
        Timer0 模式1
    */

    TMOD |= 0x01;


    /*
        定时初值
    */

    TH0 = 0xDC;
    TL0 = 0x00;


    /*
        开启 Timer0 中断
    */

    ET0 = 1;


    /*
        开启总中断
    */

    EA = 1;


    /*
        启动 Timer0
    */

    TR0 = 1;
}


/************************************************
 * UART 发送一个字符
 ************************************************/

void uart_send(u8 dat)
{
    /*
        清除发送完成标志
    */

    TI = 0;


    /*
        写入发送缓冲器
    */

    SBUF = dat;


    /*
        等待发送完成
    */

    while(!TI);


    /*
        清除发送完成标志
    */

    TI = 0;
}


/************************************************
 * UART 发送字符串
 ************************************************/

void uart_send_string(char *str)
{
    while(*str)
    {
        uart_send(*str);

        str++;
    }
}


/************************************************
 * 发送整数
 *
 * 支持：
 *
 * 0
 * 123
 * -123
 ************************************************/

void send_number(int num)
{
    char buf[8];

    u8 i = 0;


    /*
        处理 0
    */

    if(num == 0)
    {
        uart_send('0');

        return;
    }


    /*
        处理负数
    */

    if(num < 0)
    {
        uart_send('-');

        num = -num;
    }


    /*
        将数字转换成字符
    */

    while(num > 0)
    {
        buf[i++] = num % 10 + '0';

        num /= 10;
    }


    /*
        倒序发送

        例如：

        123

        前面得到：

        3
        2
        1

        所以倒过来发送
    */

    while(i > 0)
    {
        uart_send(buf[--i]);
    }
}


/************************************************
 * 字符串转整数
 *
 * 例如：
 *
 * "123"  -> 123
 * "0"    -> 0
 *
 * 支持负数：
 *
 * "-123" -> -123
 ************************************************/

int str_to_int(char *str)
{
    int value = 0;

    bit negative = 0;


    /*
        判断负号
    */

    if(*str == '-')
    {
        negative = 1;

        str++;
    }


    /*
        连续读取数字
    */

    while(*str >= '0' && *str <= '9')
    {
        value = value * 10 + (*str - '0');

        str++;
    }


    /*
        返回负数
    */

    if(negative)
    {
        value = -value;
    }


    return value;
}


/************************************************
 * 命令解析
 ************************************************/

void parse_command()
{
    int value;


    /*
        如果接收缓冲区溢出
        直接报错
    */

    if(rx_overflow)
    {
        uart_send_string("ERROR OVERFLOW\r\n");

        rx_overflow = 0;

        return;
    }


    /*
        显示收到的命令

        例如：

        CMD:GET_POS
    */

    uart_send_string("CMD:");

    uart_send_string(rx_buffer);

    uart_send_string("\r\n");


    /********************************************
     * SET_POS
     *
     * 例如：
     *
     * SET_POS 100
     ********************************************/

    if(strncmp(rx_buffer, "SET_POS ", 8) == 0)
    {
        value = str_to_int(&rx_buffer[8]);

        target_position = value;

        uart_send_string("OK SET_POS\r\n");

        return;
    }


    /********************************************
     * SET_VEL
     *
     * 例如：
     *
     * SET_VEL 10
     ********************************************/

    if(strncmp(rx_buffer, "SET_VEL ", 8) == 0)
    {
        value = str_to_int(&rx_buffer[8]);


        /*
            速度不允许为负数

            方向由目标位置决定
        */

        if(value < 0)
        {
            uart_send_string("ERROR VEL\r\n");
        }
        else
        {
            target_velocity = value;

            uart_send_string("OK SET_VEL\r\n");
        }

        return;
    }


    /********************************************
     * GET_POS
     ********************************************/

    if(strcmp(rx_buffer, "GET_POS") == 0)
    {
        uart_send_string("POS:");

        send_number(current_position);

        uart_send_string("\r\n");

        return;
    }


    /********************************************
     * GET_VEL
     ********************************************/

    if(strcmp(rx_buffer, "GET_VEL") == 0)
    {
        uart_send_string("VEL:");

        send_number(current_velocity);

        uart_send_string("\r\n");

        return;
    }


    /********************************************
     * STATUS
     *
     * 返回：
     *
     * TARGET:100 POS:50 VEL:10
     ********************************************/

    if(strcmp(rx_buffer, "STATUS") == 0)
    {
        uart_send_string("TARGET:");

        send_number(target_position);


        uart_send_string(" POS:");

        send_number(current_position);


        uart_send_string(" VEL:");

        send_number(current_velocity);


        uart_send_string("\r\n");

        return;
    }


    /********************************************
     * 未知命令
     ********************************************/

    uart_send_string("ERROR\r\n");
}


/************************************************
 * 主函数
 ************************************************/

void main()
{
    /*
        初始化 UART
    */

    uart_init();


    /*
        初始化 Timer0
    */

    timer0_init();


    /*
        主循环
    */

    while(1)
    {
        /*
            是否收到完整命令？
        */

        if(command_ready)
        {
            /*
                清除命令完成标志
            */

            command_ready = 0;


            /*
                解析命令
            */

            parse_command();
        }
    }
}


/************************************************
 * Timer0 中断
 *
 * 模拟电机运动
 ************************************************/

void timer0_isr() interrupt 1
{
    /*
        重新装载定时器
    */

    TH0 = 0xDC;

    TL0 = 0x00;


    /*
        定时计数
    */

    timer_count++;


    /*
        每 50 次更新一次电机状态
    */

    if(timer_count >= 50)
    {
        timer_count = 0;


        /****************************************
         * 当前位于目标位置左侧
         *
         * 正方向运动
         ****************************************/

        if(current_position < target_position)
        {
            /*
                速度为 0

                电机不动
            */

            if(target_velocity == 0)
            {
                current_velocity = 0;
            }
            else
            {
                /*
                    向目标位置移动
                */

                current_position += target_velocity;

                current_velocity = target_velocity;


                /*
                    防止超过目标位置
                */

                if(current_position >= target_position)
                {
                    current_position = target_position;

                    current_velocity = 0;
                }
            }
        }


        /****************************************
         * 当前位于目标位置右侧
         *
         * 负方向运动
         ****************************************/

        else if(current_position > target_position)
        {
            /*
                速度为 0

                电机不动
            */

            if(target_velocity == 0)
            {
                current_velocity = 0;
            }
            else
            {
                /*
                    向目标位置移动
                */

                current_position -= target_velocity;

                current_velocity = -target_velocity;


                /*
                    防止超过目标位置
                */

                if(current_position <= target_position)
                {
                    current_position = target_position;

                    current_velocity = 0;
                }
            }
        }


        /****************************************
         * 已经到达目标位置
         ****************************************/

        else
        {
            current_velocity = 0;
        }
    }
}


/************************************************
 * UART 中断
 *
 * 只负责接收数据
 *
 * 不负责解析命令
 ************************************************/

void uart_isr() interrupt 4
{
    char rcv_data;


    /*
        是否收到数据？
    */

    if(RI)
    {
        /*
            清除接收标志
        */

        RI = 0;


        /*
            读取接收到的字符
        */

        rcv_data = SBUF;


        /****************************************
         * 收到回车或者换行
         *
         * 表示一条命令结束
         ****************************************/

        if(rcv_data == '\r' || rcv_data == '\n')
        {
            /*
                防止空命令
            */

            if(rx_index > 0)
            {
                /*
                    添加字符串结束符

                    例如：

                    GET_POS\0
                */

                rx_buffer[rx_index] = '\0';


                /*
                    通知主循环：

                    一条完整命令已经收到
                */

                command_ready = 1;


                /*
                    准备接收下一条命令
                */

                rx_index = 0;
            }
        }


        /****************************************
         * 普通字符
         ****************************************/

        else
        {
            /*
                如果之前没有发生溢出
            */

            if(!rx_overflow)
            {
                /*
                    留一个位置给 '\0'
                */

                if(rx_index < RX_BUFFER_SIZE - 1)
                {
                    rx_buffer[rx_index++] = rcv_data;
                }
                else
                {
                    /*
                        缓冲区已满
                    */

                    rx_overflow = 1;
                }
            }
        }
    }
}