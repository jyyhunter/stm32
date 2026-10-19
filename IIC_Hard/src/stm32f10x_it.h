/**
  ******************************************************************************
  * @file    stm32f10x_it.h
  * @brief   中断处理函数声明。
  ******************************************************************************
  */
#ifndef __STM32F10x_IT_H
#define __STM32F10x_IT_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f10x.h"

/* 内核异常（启动文件弱符号引用，需提供实现） */
void NMI_Handler(void);
void HardFault_Handler(void);
void MemManage_Handler(void);
void BusFault_Handler(void);
void UsageFault_Handler(void);
void SVC_Handler(void);
void DebugMon_Handler(void);
void PendSV_Handler(void);
void SysTick_Handler(void);

/* 外设中断：用到时在此声明并在 stm32f10x_it.c 实现，例如 */
/* void USART1_IRQHandler(void); */
/* void EXTI0_IRQHandler(void); */

#ifdef __cplusplus
}
#endif

#endif /* __STM32F10x_IT_H */
