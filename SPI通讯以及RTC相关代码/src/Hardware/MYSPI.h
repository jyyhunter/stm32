#ifndef _MYSPI_H
#define _MYSPI_H
#include "stm32f10x.h"

void MYSPI_Init(void);
void mySPI_Start(void);
void mySPI_Stop(void);
uint8_t mySPI_SwapByte(uint8_t byteSend);

#endif