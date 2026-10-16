#ifndef _PWM_H_
#define _PWM_H_
#include "stm32f10x.h"                  // Device header


void PWM_SetCompare1(uint16_t Compare);
void pwm_init(void);
void delay_ms(uint32_t ms);
#endif
