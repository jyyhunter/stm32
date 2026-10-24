#pragma once
#include "stm32f10x.h"                  // Device header


void IIC_Init();
void IIC_W_SCL(uint8_t BitValue);
void IIC_W_SDA(uint8_t BitValue);
uint8_t IIC_R_SDA();
// 开启IIC通信，先拉高SDA和SCL线，然后拉低SDA线，最后拉低SCL线
void IIC_Start();
// 先拉低SDA线，然后拉低SCL线，最后拉高SDA线，最后拉高SCL线
void IIC_Stop();
// 发送一个字节数据
void IIC_SendByte(uint8_t SendByte);
// 接受一个字节数据
uint8_t IIC_ReceiveByte();
void IIC_SendAck(uint8_t AckBit);
uint8_t IIC_ReceiveAck();

void MPU_WriteReg(uint8_t reg,uint8_t data);
uint8_t MPU_ReadReg(uint8_t reg);
void MPU_Init();
uint8_t MPU_GetWhoAmI();