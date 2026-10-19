/**
  ******************************************************************************
  * @file    stm32f10x_it.c
  * @brief   中断服务函数。内核异常在此实现，外设中断按需添加。
  ******************************************************************************
  */
#include "stm32f10x_it.h"

/* ---- 内核异常 ---- */
void NMI_Handler(void) {}

void HardFault_Handler(void)
{
    while (1) {}
}

void MemManage_Handler(void)
{
    while (1) {}
}

void BusFault_Handler(void)
{
    while (1) {}
}

void UsageFault_Handler(void)
{
    while (1) {}
}

void SVC_Handler(void) {}
void DebugMon_Handler(void) {}
void PendSV_Handler(void) {}
void SysTick_Handler(void) {}

/* =====================================================================
 * 外设中断模板（取消注释并实现，注意函数名必须与启动文件向量表一致）
 * ===================================================================== */
/*
void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
    {
        uint16_t data = USART_ReceiveData(USART1);
        // 处理 data ...
        USART_ClearITPendingBit(USART1, USART_IT_RXNE);
    }
}
*/
