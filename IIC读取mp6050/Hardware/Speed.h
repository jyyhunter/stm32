#ifndef __SPEED_H
#define __SPEED_H

#include "stm32f10x.h"

/* ============================================================
 *  AS5600 速度测量模块（M法测速 + 滑动窗口）
 *
 *  原理：
 *    1. 每固定周期 SPEED_SAMPLE_MS(50ms) 读一次 AS5600 原始角度；
 *    2. 与上一次角度做差，并处理 0/4095 回绕，得到带符号增量；
 *       增量 > 2047 视为反向过零点（减 4096），反之加 4096；
 *    3. 把最近 SPEED_WINDOW_N(4) 个增量（共 200ms）做滑动求和，
 *       降低低速时的量化跳动；
 *    4. 由窗口内累计的角度增量换算 RPM 与角速度。
 *
 *  注意：单圈绝对编码器在一个采样周期内转过的角度不能超过半圈
 *       （2048 个刻度，即 50ms 内不超过半圈，量程约 ±600rpm），
 *       否则过零点方向会判断错误。
 *
 *  全部使用整数定点运算，避免依赖浮点 printf。
 * ============================================================ */

#define SPEED_SAMPLE_MS    50U   /* 采样周期 ms */
#define SPEED_WINDOW_N     4U    /* 滑动窗口个数，窗口 = 4*50ms = 200ms */
#define COUNTS_PER_REV     4096  /* AS5600 一圈刻度数（12位） */

void     Speed_Init(void);            /* 初始化 AS5600 并建立基准 */
void     Speed_Sample(void);          /* 每 SPEED_SAMPLE_MS 调用一次 */

int32_t  Speed_GetRpmCenti(void);     /* 转速(转/分) ×100，带符号；如 12345 表示 123.45 rpm */
int32_t  Speed_GetDpsTenth(void);     /* 角速度(度/秒) ×10，带符号；如 3601 表示 360.1 度/秒 */
int32_t  Speed_GetAngleCenti(void);   /* 当前绝对角度(度) ×100，0~35999；如 9000 表示 90.00 度 */
int32_t  Speed_GetTotalCount(void);   /* 上电后累计位置(刻度)，带符号，可用于多圈 */
int32_t  Speed_GetRevolutions(void);  /* 上电后累计圈数，带符号 */
uint8_t  Speed_GetStatus(void);       /* AS5600 状态寄存器：bit5=MD磁铁在位 bit4=ML过弱 bit3=MH过强 */

#endif
