#ifndef __PID_H
#define __PID_H

#include "stm32f10x.h"

typedef struct {
    float P;
    float I;
    float D;
    float output_ramp;     /* 输出变化率限制(单位/秒)，0=不限制 */
    float limit;           /* 输出限幅 */
    float error_prev;
    float output_prev;
    float integral_prev;
} PIDController;

void  PID_Init(PIDController *pid, float P, float I, float D, float ramp, float limit);
void  PID_Reset(PIDController *pid);
float PID_Operator(PIDController *pid, float error, float Ts);

#endif
