#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

/* 调试串口 USART3
 *   TX : PB10
 *   RX : PB11
 * 支持 printf 重定向（fputc） */
void Serial_Init(uint32_t baud);

#endif
