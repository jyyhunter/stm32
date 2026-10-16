#pragma once
#include "stm32f10x.h"                  // Device header
#include <stdio.h>
void Serial_Init();
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);


int fpuc(int ch, FILE* f);
// 可变参数的结构
void Serial_Printf(char * format,...);
