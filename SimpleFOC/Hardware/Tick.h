#ifndef __TICK_H
#define __TICK_H

#include "stm32f10x.h"

/* TIM2 产生 1ms 周期中断，用于 FOC 控制节拍 */
void Tick_Init(void);

extern volatile uint32_t g_tick_ms;   /* 毫秒计数 */
extern volatile uint8_t  g_foc_tick;  /* 每个1ms置1，主循环清零后执行FOC */

#endif
