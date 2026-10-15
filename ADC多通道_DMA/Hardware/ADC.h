#ifndef _ADC_H
#define _ADC_H
#include "stm32f10x.h"                  // Device header

void AD_Init();
//void AD_GetValue();
extern uint16_t AD_Value[4];
#endif
