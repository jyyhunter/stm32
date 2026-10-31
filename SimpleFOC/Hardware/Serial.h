#ifndef __SERIAL_H
#define __SERIAL_H

#include "stm32f10x.h"

/* 调试串口 USART3
 *   TX : PB10
 *   RX : PB11
 * 支持 printf 重定向（fputc） */
void Serial_Init(uint32_t baud);
void Serial_Write(const uint8_t *data, uint16_t length);
void Serial_WriteString(const char *text);
uint8_t Serial_ReadByte(uint8_t *value);

#endif
