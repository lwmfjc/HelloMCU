#include "usart.h"
#include <string.h>


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


/*
 * volatile 非常重要
 *
 * 因为这些变量会被：
 *
 * 主程序
 *
 * 和
 *
 * USART中断
 *
 * 共同访问
 */

volatile u8 rx_index = 0;

volatile u8 command_ready = 0;

volatile u8 rx_overflow = 0;


/************************************************
 * USART1 初始化
 *
 * PA9  -> TX
 * PA10 -> RX
 *
 * 9600 8N1
 ************************************************/

void USART1_Init(u32 bound)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;


    /********************************************
     * 开启GPIOA时钟
     ********************************************/

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_GPIOA,
        ENABLE
    );


    /********************************************
     * 开启USART1时钟
     ********************************************/

    RCC_APB2PeriphClockCmd(
        RCC_APB2Periph_USART1,
        ENABLE
    );


    /********************************************
     * PA9 -> USART1_TX
     ********************************************/

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;

    GPIO_InitStructure.GPIO_Speed =
        GPIO_Speed_50MHz;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_AF_PP;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /********************************************
     * PA10 -> USART1_RX
     ********************************************/

    GPIO_InitStructure.GPIO_Pin =
        GPIO_Pin_10;

    GPIO_InitStructure.GPIO_Mode =
        GPIO_Mode_IN_FLOATING;

    GPIO_Init(GPIOA, &GPIO_InitStructure);


    /********************************************
     * USART1参数
     *
     * 9600
     * 8位数据
     * 1停止位
     * 无校验
     ********************************************/

    USART_InitStructure.USART_BaudRate =
        bound;

    USART_InitStructure.USART_WordLength =
        USART_WordLength_8b;

    USART_InitStructure.USART_StopBits =
        USART_StopBits_1;

    USART_InitStructure.USART_Parity =
        USART_Parity_No;

    USART_InitStructure.USART_HardwareFlowControl =
        USART_HardwareFlowControl_None;

    USART_InitStructure.USART_Mode =
        USART_Mode_Rx | USART_Mode_Tx;


    USART_Init(
        USART1,
        &USART_InitStructure
    );


    /********************************************
     * 使能USART1
     ********************************************/

    USART_Cmd(
        USART1,
        ENABLE
    );


    /********************************************
     * 开启接收中断
     *
     * RXNE：
     *
     * Receive Data Register Not Empty
     ********************************************/

    USART_ITConfig(
        USART1,
        USART_IT_RXNE,
        ENABLE
    );


    /********************************************
     * USART1 NVIC配置
     ********************************************/

    NVIC_InitStructure.NVIC_IRQChannel =
        USART1_IRQn;

    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority =
        3;

    NVIC_InitStructure.NVIC_IRQChannelSubPriority =
        3;

    NVIC_InitStructure.NVIC_IRQChannelCmd =
        ENABLE;


    NVIC_Init(
        &NVIC_InitStructure
    );
}


/************************************************
 * UART发送一个字符
 ************************************************/

void uart_send(u8 dat)
{
    /*
     * 把数据写入USART1发送寄存器
     */

    USART_SendData(
        USART1,
        dat
    );


    /*
     * 等待发送完成
     */

    while(
        USART_GetFlagStatus(
            USART1,
            USART_FLAG_TC
        ) == RESET
    );
}


/************************************************
 * UART发送字符串
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
    char buf[12];

    u8 i = 0;


    /********************************************
     * 处理0
     ********************************************/

    if(num == 0)
    {
        uart_send('0');

        return;
    }


    /********************************************
     * 处理负数
     ********************************************/

    if(num < 0)
    {
        uart_send('-');

        num = -num;
    }


    /********************************************
     * 数字转换成字符
     ********************************************/

    while(num > 0)
    {
        buf[i++] =
            num % 10 + '0';

        num /= 10;
    }


    /********************************************
     * 倒序发送
     *
     * 例如：
     *
     * 123
     *
     * 实际保存：
     *
     * 3
     * 2
     * 1
     *
     * 所以倒过来发送
     ********************************************/

    while(i > 0)
    {
        uart_send(
            buf[--i]
        );
    }
}


/************************************************
 * 字符串转整数
 *
 * "123"  -> 123
 * "0"    -> 0
 * "-123" -> -123
 ************************************************/

int str_to_int(char *str)
{
    int value = 0;

    u8 negative = 0;


    /********************************************
     * 判断负号
     ********************************************/

    if(*str == '-')
    {
        negative = 1;

        str++;
    }


    /********************************************
     * 连续读取数字
     ********************************************/

    while(
        *str >= '0' &&
        *str <= '9'
    )
    {
        value =
            value * 10 +
            (*str - '0');

        str++;
    }


    /********************************************
     * 返回负数
     ********************************************/

    if(negative)
    {
        value = -value;
    }


    return value;
}


/************************************************
 * 命令解析
 ************************************************/

void parse_command(void)
{
    int value;


    /********************************************
     * 缓冲区溢出
     ********************************************/

    if(rx_overflow)
    {
        uart_send_string(
            "ERROR OVERFLOW\r\n"
        );

        rx_overflow = 0;

        return;
    }


    /********************************************
     * 显示收到的命令
     *
     * 例如：
     *
     * CMD:GET_POS
     ********************************************/

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

    if(
        strncmp(
            rx_buffer,
            "SET_POS ",
            8
        ) == 0
    )
    {
        value =
            str_to_int(
                &rx_buffer[8]
            );


        target_position = value;


        uart_send_string(
            "OK SET_POS\r\n"
        );


        return;
    }


    /********************************************
     * SET_VEL
     *
     * 例如：
     *
     * SET_VEL 10
     ********************************************/

    if(
        strncmp(
            rx_buffer,
            "SET_VEL ",
            8
        ) == 0
    )
    {
        value =
            str_to_int(
                &rx_buffer[8]
            );


        /*
         * 速度不能为负数
         */

        if(value < 0)
        {
            uart_send_string(
                "ERROR VEL\r\n"
            );
        }
        else
        {
            target_velocity = value;


            uart_send_string(
                "OK SET_VEL\r\n"
            );
        }


        return;
    }


    /********************************************
     * GET_POS
     ********************************************/

    if(
        strcmp(
            rx_buffer,
            "GET_POS"
        ) == 0
    )
    {
        uart_send_string(
            "POS:"
        );


        send_number(
            current_position
        );


        uart_send_string(
            "\r\n"
        );


        return;
    }


    /********************************************
     * GET_VEL
     ********************************************/

    if(
        strcmp(
            rx_buffer,
            "GET_VEL"
        ) == 0
    )
    {
        uart_send_string(
            "VEL:"
        );


        send_number(
            current_velocity
        );


        uart_send_string(
            "\r\n"
        );


        return;
    }


    /********************************************
     * STATUS
     *
     * 返回：
     *
     * TARGET:100 POS:50 VEL:10
     ********************************************/

    if(
        strcmp(
            rx_buffer,
            "STATUS"
        ) == 0
    )
    {
        uart_send_string(
            "TARGET:"
        );


        send_number(
            target_position
        );


        uart_send_string(
            " POS:"
        );


        send_number(
            current_position
        );


        uart_send_string(
            " VEL:"
        );


        send_number(
            current_velocity
        );


        uart_send_string(
            "\r\n"
        );


        return;
    }


    /********************************************
     * 未知命令
     ********************************************/

    uart_send_string(
        "ERROR\r\n"
    );
}


/************************************************
 * USART1中断
 *
 * 这里只负责：
 *
 * 1. 接收字符
 * 2. 放入缓存
 * 3. 判断命令是否结束
 *
 * 不负责解析命令
 ************************************************/

void USART1_IRQHandler(void)
{
    char rcv_data;


    /********************************************
     * 检查RXNE
     ********************************************/

    if(
        USART_GetITStatus(
            USART1,
            USART_IT_RXNE
        ) != RESET
    )
    {
        /****************************************
         * 读取接收到的字符
         *
         * 读取DR之后，
         * RXNE会自动清除
         ****************************************/

        rcv_data =
            USART_ReceiveData(
                USART1
            );


        /****************************************
         * 收到回车或者换行
         *
         * 表示一条命令结束
         ****************************************/

        if(
            rcv_data == '\r' ||
            rcv_data == '\n'
        )
        {
            /*
             * 防止空命令
             */

            if(rx_index > 0)
            {
                /*
                 * 添加字符串结束符
                 */

                rx_buffer[rx_index] =
                    '\0';


                /*
                 * 通知主循环：
                 *
                 * 一条完整命令已经收到
                 */

                command_ready = 1;


                /*
                 * 从头开始接收下一条命令
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
             * 如果没有溢出
             */

            if(!rx_overflow)
            {
                /*
                 * 留一个位置给 '\0'
                 */

                if(
                    rx_index <
                    RX_BUFFER_SIZE - 1
                )
                {
                    rx_buffer[rx_index++] =
                        rcv_data;
                }
                else
                {
                    /*
                     * 缓冲区已满
                     */

                    rx_overflow = 1;
                }
            }
        }
    }
}