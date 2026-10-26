	#ifndef __AS5600_H
#define __AS5600_H

#include "stm32f10x.h"

/* ============ AS5600 软件I2C 引脚 ============
 *   SCL : PB6
 *   SDA : PB7
 *   VCC : 3.3V    GND : GND
 *   DIR : 磁编码器方向(可在FOC里用软件方向代替)
 * ============================================ */

void     AS5600_Init(void);          /* 初始化，返回0成功，非0失败 */
uint16_t AS5600_GetRawAngle(void);   /* 读取原始角度 0~4095（12位） */
float    AS5600_GetMechAngle(void);  /* 读取机械角度 0~2pi (rad) */
uint8_t  AS5600_ReadReg(uint8_t reg);/* 读取单个8位寄存器 */
uint8_t  AS5600_GetStatus(void);     /* 读状态，bit5=MD磁铁检测，bit4=ML过弱，bit3=MH过强 */

#endif
