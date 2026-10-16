#pragma once
#include "stm32f10x.h"                  // Device header
#include <stdio.h>
void Serial_Init();
void Serial_SendByte(uint8_t Byte);
void Serial_SendArray(uint8_t *Array,uint16_t Length);
void Serial_SendPacket();
uint8_t GetRx_Flag();

int fpuc(int ch, FILE* f);
// 可变参数的结构
void Serial_Printf(char * format,...);
void Serial_SendNumber(uint32_t Number, uint8_t Length);
uint32_t Serial_pow(uint32_t x, uint32_t y);
void Serial_SendString(char * String);
extern char Serial_RxPacket[100];