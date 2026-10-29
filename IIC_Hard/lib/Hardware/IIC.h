#ifndef __IIC_H
#define __IIC_H

#include "stm32f10x.h"

void IIC_Init(void);
void MPU_WriteReg(uint8_t reg, uint8_t data);

#endif