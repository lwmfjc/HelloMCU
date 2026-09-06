#ifndef __MOTOR_H
#define __MOTOR_H

#include "system.h"
#include "SysTick.h"


/*
 * 步进电机 GPIO
 *
 * 原51：
 * P1.0 -> IA
 * P1.1 -> IB
 * P1.2 -> IC
 * P1.3 -> ID
 *
 * 目前按照 PA0~PA3
 */
#define MOTOA    PAout(0)
#define MOTOB    PAout(1)
#define MOTOC    PAout(2)
#define MOTOD    PAout(3)


/*
 * 电机参数
 */
extern u16 SPEED;

extern u8 direction;

extern u8 motor_run;


/*
 * 串口命令计数器
 */
extern volatile u8 cmd_1_count;
extern volatile u8 cmd_2_count;
extern volatile u8 cmd_3_count;
extern volatile u8 cmd_0_count;
extern volatile u8 cmd_s_count;


/*
 * 电机函数
 */
void Motor_Init(void);

void motor_forward(void);

void motor_reverse(void);

void motor_stop(void);

void process_command(void);


#endif