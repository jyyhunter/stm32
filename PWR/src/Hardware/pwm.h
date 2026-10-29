#ifndef _PWM_H_
#define _PWM_H_
#include "stm32f10x.h"

// 通用 PWM 通道描述
typedef struct {
    TIM_TypeDef *TIMx;      // 定时器
    uint8_t      ch;        // 1~4
    uint16_t     psc;       // 预分频
    uint16_t     arr;       // 周期
    uint16_t     ccr;       // 初始占空比
} PWM_Ch;


void PWM_Init();
void PWMCommon_Init(PWM_Ch *c);
#endif