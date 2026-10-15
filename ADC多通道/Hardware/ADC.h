#ifndef _ADC_H
#define _ADC_H
#include "stm32f10x.h"                  // Device header

void AD_Init();
uint16_t AD_GetValue(uint8_t ADC_channel);

#endif
