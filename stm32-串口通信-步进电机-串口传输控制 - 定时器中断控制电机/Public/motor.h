#ifndef __MOTOR_H
#define __MOTOR_H

#include "system.h"

/* 电机四个控制引脚 */
#define MOTOA    PAout(0)
#define MOTOB    PAout(1)
#define MOTOC    PAout(2)
#define MOTOD    PAout(3)

/* 速度参数 */
#define SPEED_MIN  1000
#define SPEED_MAX  5000
#define SPEED_STEP 200

/* 电机状态 */
extern u16 SPEED;
extern u8 direction;
extern u8 motor_run;

/* 串口命令计数 */
extern volatile u8 cmd_1_count;
extern volatile u8 cmd_2_count;
extern volatile u8 cmd_3_count;
extern volatile u8 cmd_0_count;
extern volatile u8 cmd_s_count;

/* 电机函数 */
void Motor_Init(void);
void motor_stop(void);
void process_command(void);

/* Timer调用的步进函数 */
void motor_step(void);

#endif