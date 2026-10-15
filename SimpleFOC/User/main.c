#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "Serial.h"
#include "Tick.h"
#include "BLDCDriver.h"
#include "AS5600.h"
#include "FOC.h"

/* ===================== 用户配置 ===================== */
#define POWER_SUPPLY_VOLTAGE   12.0f    /* 驱动电源电压(V)，按实际供电改 */
#define POLE_PAIRS             7        /* 电机极对数（云台电机常见 7） */
#define ENABLE_DEMO            1        /* 1: 每3秒在 0 / 1.5rad 间切换演示；0: 锁定在0点 */
/* ==================================================== */

BLDCDriver g_driver;
FOCMotor   g_motor;

int main(void)
{
    uint8_t res;

    Serial_Init(115200);
    Delay_ms(500);
    printf("=== SimpleFOC STM32F103VET6 ===\r\n");

    /* 初始化传感器、驱动、电机 */
    AS5600_Init();
    Driver_Init(&g_driver, POWER_SUPPLY_VOLTAGE);
    FOC_Init(&g_motor, POLE_PAIRS, POWER_SUPPLY_VOLTAGE);
    g_motor.driver = &g_driver;

    Driver_Enable(&g_driver);

    /* 自动对齐（检测方向 + 记录零位），期间电机会锁定并轻微转动 */
    printf("Aligning...\r\n");
    res = FOC_Align(&g_motor);
    if (res != 0)
    {
        printf("Align FAILED, code=%d (check AS5600 wiring/magnet)\r\n", res);
        while (1) ;
    }
    printf("Align OK. dir=%.0f, zero=%.3f\r\n",
           g_motor.sensor_direction, g_motor.zero_electric_angle);

    g_motor.mode = MOTOR_MODE_ANGLE;
    g_motor.target_angle = 0.0f;

    /* 启动 1ms 控制节拍 */
    Tick_Init();

    while (1)
    {
        if (g_foc_tick)
        {
            g_foc_tick = 0;
            FOC_Loop(&g_motor, 0.001f);

#if ENABLE_DEMO
            /* 演示：每 3 秒切换目标角度 */
            {
                static uint32_t last = 0;
                static uint8_t  t = 0;
                if (g_tick_ms - last > 3000)
                {
                    last = g_tick_ms;
                    t = !t;
                    g_motor.target_angle = t ? 1.5f : 0.0f;
                }
            }
#endif
        }
    }
}
