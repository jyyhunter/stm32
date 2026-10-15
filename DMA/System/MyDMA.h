#ifndef _MY_DMA_H
#define _MY_DMA_H
#include "stm32f10x.h"                  // Device header

void MYDMA_Init(uint32_t AdddrA,uint32_t ADDRB,uint16_t Size);
void MyDMA_Transfer();

#endif
