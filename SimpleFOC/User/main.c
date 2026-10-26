#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "Serial.h"
#include "Tick.h"
#include "BLDCDriver.h"
#include "AS5600.h"
#include "FOC.h"

/* ===================== 用户配置 ===================== */
#define POWER_SUPPLY_VOLTAGE   12.0f    /* 驱动电源电压(V)，必须与实测母线一致 */
#define POLE_PAIRS             7        /* 电机极对数（云台电机常见 7） */
#define TARGET_SPEED_RPM       30.0f    /* 目标机械转速(RPM)，负值表示反转 */
#define TWO_PI                 6.28318530718f
#define TEST_OPEN_LOOP         0        /* 1: 开环速度测试；0: 编码器闭环速度 */
#define OPEN_LOOP_VOLTAGE      6.0f     /* 开环 q 轴电压(V)，从低值开始调试 */
#define OPEN_LOOP_TEST_MS      10000U   /* 自动停机时间，防止堵转持续发热 */
#define ALIGN_VOLTAGE          6.0f     /* 对齐电压(V)，3 V 无法带动本电机 */
#define MAX_PHASE_VOLTAGE      6.0f     /* 闭环 q 轴电压限制(V) */
#define SPEED_OUTPUT_RAMP      30.0f    /* 速度环输出电压上升率(V/s) */
#define CLOSED_LOOP_TEST_MS    10000U   /* 闭环测试自动停机时间 */
#define SAFE_STOP_MODE         1        /* 1: 禁止电机输出；排查发热后改为0 */
/* ==================================================== */

BLDCDriver g_driver;
FOCMotor   g_motor;

int main(void)
{
#if !SAFE_STOP_MODE
#if !TEST_OPEN_LOOP
    uint8_t res;
#endif
#if TEST_OPEN_LOOP
    float electrical_angle = 0.0f;
#endif
#endif

    Serial_Init(115200);
    Delay_ms(500);
    printf("=== SimpleFOC STM32F103VET6 Velocity Demo ===\r\n");
    /* 初始化传感器、驱动、电机 */
    AS5600_Init();
    Driver_Init(&g_driver, POWER_SUPPLY_VOLTAGE);
    FOC_Init(&g_motor, POLE_PAIRS, POWER_SUPPLY_VOLTAGE);
    g_motor.driver = &g_driver;
    g_motor.voltage_sensor_align = ALIGN_VOLTAGE;
    g_motor.voltage_limit = MAX_PHASE_VOLTAGE;
    g_motor.P_velocity.limit = MAX_PHASE_VOLTAGE;
    g_motor.P_velocity.output_ramp = SPEED_OUTPUT_RAMP;

#if SAFE_STOP_MODE
    uint16_t raw_angle = 0xFFFF;
    uint16_t last_raw_angle = 0xFFFE;
    uint8_t sensor_status = 0xFF;
    uint8_t last_sensor_status = 0xFE;

    Driver_Disable(&g_driver);
    printf("SAFE STOP: motor output disabled; encoder test enabled.\r\n");
    printf("Rotate the shaft manually.\r\n");
    while (1)
    {
        raw_angle = AS5600_GetRawAngle();
        sensor_status = AS5600_GetStatus();

        if (raw_angle != last_raw_angle || sensor_status != last_sensor_status)
        {
            if (raw_angle <= 0x0FFF)
            {
                printf("raw=%u angle=%.3f rad status=0x%02X\r\n",
                       (unsigned int)raw_angle,
                       (float)raw_angle * TWO_PI / 4096.0f,
                       (unsigned int)sensor_status);
            }
            else
            {
                printf("AS5600 read failed: NACK or invalid angle\r\n");
            }
            last_raw_angle = raw_angle;
            last_sensor_status = sensor_status;
        }
        Delay_ms(100);
    }
#endif

#if !SAFE_STOP_MODE
    Driver_Enable(&g_driver);

#if TEST_OPEN_LOOP
    const float electrical_step =
        TARGET_SPEED_RPM * TWO_PI / 60.0f * POLE_PAIRS * 0.001f;
    float mech_start = g_motor.mech_prev;
    uint32_t test_start;
    uint32_t elapsed;

    /* 不依赖编码器对齐，直接按目标机械转速旋转电角度 */
    printf("Open-loop speed: %.1f RPM, %.1f V, timeout %u ms\r\n",
           TARGET_SPEED_RPM, OPEN_LOOP_VOLTAGE,
           (unsigned int)OPEN_LOOP_TEST_MS);
    Tick_Init();
    test_start = g_tick_ms;
    while (1)
    {
        if (g_foc_tick)
        {
            float voltage_scale;

            g_foc_tick = 0;
            elapsed = g_tick_ms - test_start;
            if (elapsed >= OPEN_LOOP_TEST_MS)
            {
                Driver_Disable(&g_driver);
                printf("Open-loop test finished. sensor start=%.3f end=%.3f\r\n",
                       mech_start, g_motor.mech_prev);
                while (1) ;
            }

            electrical_angle += electrical_step;
            if (electrical_angle >= TWO_PI)
                electrical_angle -= TWO_PI;
            else if (electrical_angle < 0.0f)
                electrical_angle += TWO_PI;

            /* 前 1 秒线性升压，降低启动冲击和堵转电流 */
            voltage_scale = (elapsed < 1000U) ? elapsed * 0.001f : 1.0f;
            FOC_SetPhaseVoltage(&g_motor, OPEN_LOOP_VOLTAGE * voltage_scale,
                                0.0f, electrical_angle);
            g_motor.mech_prev = AS5600_GetMechAngle();
        }
    }
#else
    /* 自动对齐（检测方向 + 记录零位），期间电机会锁定并轻微转动 */
    printf("Aligning...\r\n");
    res = FOC_Align(&g_motor);
    if (res != 0)
    {
        printf("Align FAILED, code=%d (check AS5600 wiring/magnet)\r\n", res);
        Driver_Disable(&g_driver);
        while (1) ;
    }
    printf("Align OK. dir=%.0f, zero=%.3f\r\n",
           g_motor.sensor_direction, g_motor.zero_electric_angle);

    /* 速度闭环：target_velocity 的单位为机械角速度 rad/s */
    g_motor.mode = MOTOR_MODE_VELOCITY;
    g_motor.target_velocity = TARGET_SPEED_RPM * TWO_PI / 60.0f;
    printf("Target speed: %.1f RPM (%.3f rad/s)\r\n",
           TARGET_SPEED_RPM, g_motor.target_velocity);

    /* 启动 1ms 控制节拍 */
    Tick_Init();
    {
        uint32_t closed_loop_start = g_tick_ms;

        while (1)
        {
            if (g_foc_tick)
            {
                g_foc_tick = 0;
                FOC_Loop(&g_motor, 0.001f);
                if (g_tick_ms - closed_loop_start >= CLOSED_LOOP_TEST_MS)
                {
                    Driver_Disable(&g_driver);
                    printf("Closed-loop test finished. target=%.3f actual=%.3f\r\n",
                           g_motor.target_velocity, g_motor.shaft_velocity);
                    while (1) ;
                }
            }
        }
    }
#endif
#endif
}
