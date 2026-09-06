#ifndef __USART_H
#define __USART_H

#include "system.h"


/************************************************
 * 串口接收状态变量
 ************************************************/

/*
 * 这些变量实际定义在 usart.c
 *
 * main.c 通过 extern 使用它们
 */

extern volatile u8 rx_index;

extern volatile u8 command_ready;

extern volatile u8 rx_overflow;

extern int target_position;
extern int current_position;
extern int target_velocity;
extern int current_velocity;


/************************************************
 * USART1
 ************************************************/

void USART1_Init(u32 bound);


/************************************************
 * UART发送函数
 ************************************************/

void uart_send(u8 dat);

void uart_send_string(char *str);

void send_number(int num);


/************************************************
 * 命令解析
 ************************************************/

void parse_command(void);


#endif