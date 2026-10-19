#pragma once

#include "stm32f10x.h"

void IIC_Init(void);
void MPU_WriteReg(uint8_t reg, uint8_t data);

