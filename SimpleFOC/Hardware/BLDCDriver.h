#ifndef __BLDCDRIVER_H
#define __BLDCDRIVER_H

#include "stm32f10x.h"

/* ===================== 引脚定义(SimpleFOC Mini / DRV8313, 3PWM+1使能) =====================
 * 三相PWM 使用高级定时器 TIM1（直接接驱动板 IN1/IN2/IN3）
 *   IN1(U相) : PA8  -> TIM1_CH1
 *   IN2(V相) : PA9  -> TIM1_CH2
 *   IN3(W相) : PA10 -> TIM1_CH3
 * 驱动器总使能 EN 使用普通推挽GPIO（高电平使能）
 *   EN : PB15
 * 其余控制脚（低有效，直接硬件处理即可）：
 *   nRT(复位) : 直接接 3.3V（保持不复位）
 *   nSP(休眠) : 直接接 3.3V（保持不休眠）
 *   nFT(故障) : 开漏输出，可悬空不接；要读故障时经上拉接 MCU 输入
 * 若接线不同，改这里的端口/引脚即可（PWM 必须在 TIM1 的 CH1/2/3 上）。
 * ================================================================== */
#define DRIVER_EN_PORT      GPIOB
#define DRIVER_EN_PIN       GPIO_Pin_15

/* PWM 开关频率 20kHz（人耳听不到，云台常用）。TIM1 时钟 72MHz，周期 3600 */
#define DRIVER_PWM_FREQ     20000.0f
#define DRIVER_PWM_PERIOD   (uint32_t)(72000000.0f / DRIVER_PWM_FREQ)  /* 3600 */

typedef struct {
    float voltage_power_supply;   /* 母线/电源电压，例如 12.0f */
    float voltage_limit;          /* 驱动允许输出的最大电压，默认等于电源电压 */
} BLDCDriver;

/* 初始化驱动，Vpower 为电源电压(V) */
void Driver_Init(BLDCDriver *driver, float Vpower);
/* 使能驱动（EN 拉高） */
void Driver_Enable(BLDCDriver *driver);
/* 失能驱动（EN 拉低，PWM清零） */
void Driver_Disable(BLDCDriver *driver);
/* 直接设置三相桥臂电压，Ua/Ub/Uc 范围 0 ~ voltage_power_supply（已含中点偏移） */
void Driver_SetPwm(BLDCDriver *driver, float Ua, float Ub, float Uc);

#endif
