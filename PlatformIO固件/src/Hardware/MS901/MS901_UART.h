#ifndef __MS901_UART_H
#define __MS901_UART_H

/*
 * ATK-MS901M 模块串口：USART3 (PB10=TX, PB11=RX)，中断接收 + 环形缓冲
 * 主循环从 g_ms901_rx 取字节，交给 atk_ms901m_parse_byte() 解析。
 */

#include "stm32f10x.h"
#include "RingBuffer.h"

#define MS901_RXBUF_SIZE    128

extern RingBuffer_t g_ms901_rx;

void MS901_UART_Init(uint32_t baudrate);

#endif
