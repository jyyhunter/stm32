#ifndef __BLDCDRIVER_H
#define __BLDCDRIVER_H

#include "stm32f10x.h"

/* ===================== 引脚定义(3PWM + 3使能) =====================
 * 三相PWM 使用高级定时器 TIM1
 *   U相 : PA8  -> TIM1_CH1
 *   V相 : PA9  -> TIM1_CH2
 *   W相 : PA10 -> TIM1_CH3
 * 三相使能(方向)脚 使用普通推挽GPIO
 *   U_EN : PB13
 *   V_EN : PB14
 *   W_EN : PB15
 * 若你的板子接线不同，改这里的端口/引脚即可（PWM必须在TIM1的CH1/2/3上）。
 * ================================================================== */
#define DRIVER_EN_PORT      GPIOB
#define DRIVER_EN_U_PIN     GPIO_Pin_13
#define DRIVER_EN_V_PIN     GPIO_Pin_14
#define DRIVER_EN_W_PIN     GPIO_Pin_15
#define DRIVER_EN_ALL_PINS  (DRIVER_EN_U_PIN | DRIVER_EN_V_PIN | DRIVER_EN_W_PIN)

/* PWM 开关频率 20kHz（人耳听不到，云台常用）。TIM1 时钟 72MHz，周期 3600 */
#define DRIVER_PWM_FREQ     20000.0f
#define DRIVER_PWM_PERIOD   (uint32_t)(72000000.0f / DRIVER_PWM_FREQ)  /* 3600 */

typedef struct {
    float voltage_power_supply;   /* 母线/电源电压，例如 12.0f */
    float voltage_limit;          /* 驱动允许输出的最大电压，默认等于电源电压 */
} BLDCDriver;

/* 初始化驱动，Vpower 为电源电压(V) */
void Driver_Init(BLDCDriver *driver, float Vpower);
/* 使能驱动（三个使能脚拉高） */
void Driver_Enable(BLDCDriver *driver);
/* 失能驱动（三个使能脚拉低，PWM清零） */
void Driver_Disable(BLDCDriver *driver);
/* 直接设置三相桥臂电压，Ua/Ub/Uc 范围 0 ~ voltage_power_supply（已含中点偏移） */
void Driver_SetPwm(BLDCDriver *driver, float Ua, float Ub, float Uc);

#endif
