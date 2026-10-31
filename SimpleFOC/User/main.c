#include "stm32f10x.h"
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
#define OPEN_LOOP_TEST_MS      100000U   /* 自动停机时间，防止堵转持续发热 */
#define ALIGN_VOLTAGE          4.0f     /* 低速测试使用较低对齐电压(V) */
#define MAX_PHASE_VOLTAGE      6.0f     /* 12 V 母线下的闭环限压(V) */
#define SPEED_OUTPUT_RAMP      3.0f     /* 速度环输出电压上升率(V/s) */
#define SPEED_COMMAND_RAMP     1.5f     /* 目标速度上升率(rad/s²) */
#define SPEED_KP               0.25f    /* 速度环比例增益 */
#define SPEED_KI               0.5f     /* 速度环积分增益 */
#define OPENLOOP_UQ_BASE       0.50f    /* 12 V V/f 模式基础电压(V) */
#define OPENLOOP_UQ_PER_RPM    0.065f   /* 12 V 高转速段 V/f 增益 */
#define OPENLOOP_START_UQ      6.00f    /* 静止启动阶段最高补偿电压(V) */
#define OPENLOOP_RAMP_MS       100U     /* 起动电压上升时间 */
#define OPENLOOP_START_TIMEOUT 2000U    /* 起动超时时间 */
#define STALL_TIMEOUT_MS       1000U    /* 低速启动允许时间，之后检测堵转 */
#define MOTOR_TORQUE_SIGN      1.0f     /* 对齐基准修正后使用正转矩方向 */
#define SAFE_STOP_MODE         0        /* 1: 禁止电机输出；排查发热后改为0 */
#define REMOTE_RPM_LIMIT       90.0f     /* 12 V 供电目标速度上限(RPM) */
#define REMOTE_POSITION_LIMIT 720.0f    /* 相对零位的位置上限(deg) */
#define REMOTE_HEARTBEAT_MS    3000U    /* 上位机断线停机时间 */
#define TELEMETRY_PERIOD_MS    100U     /* 遥测周期 */
/* ==================================================== */

BLDCDriver g_driver;
FOCMotor   g_motor;

#if !SAFE_STOP_MODE
enum {
    REMOTE_MODE_STOPPED = 0,
    REMOTE_MODE_SPEED = 1,
    REMOTE_MODE_POSITION = 2,
    REMOTE_MODE_OPENLOOP = 3,
    REMOTE_MODE_TORQUE = 4
};

static char command_line[64];
static uint8_t command_length = 0;
static uint8_t remote_enabled = 0;
static uint8_t remote_mode = REMOTE_MODE_STOPPED;
static float requested_speed = 0.0f;
static float open_loop_electrical_angle = 0.0f;
static uint32_t open_loop_start_tick = 0;
static float open_loop_start_angle = 0.0f;
static uint8_t open_loop_started = 0;
static float slow_speed = 0.0f;
static float speed_trim = 0.0f;
static uint32_t last_heartbeat_ms = 0;
static uint32_t last_telemetry_ms = 0;
static uint32_t telemetry_speed_tick = 0;
static float telemetry_speed_angle = 0.0f;
static float telemetry_speed_rpm = 0.0f;

static float remote_clamp(float value, float low, float high)
{
    if (value < low) return low;
    if (value > high) return high;
    return value;
}

static void remote_stop(void)
{
    remote_enabled = 0;
    remote_mode = REMOTE_MODE_STOPPED;
    PID_Reset(&g_motor.P_angle);
    PID_Reset(&g_motor.P_velocity);
    g_motor.Uq = 0.0f;
    requested_speed = 0.0f;
    g_motor.target_velocity = 0.0f;
    g_motor.target_angle = g_motor.shaft_angle;
    Driver_Disable(&g_driver);
}

static void remote_start(void)
{
    g_motor.mode = MOTOR_MODE_VELOCITY;
    requested_speed = 0.0f;
    g_motor.target_velocity = 0.0f;
    remote_mode = REMOTE_MODE_SPEED;
    remote_enabled = 1;
    last_heartbeat_ms = g_tick_ms;
    Driver_Enable(&g_driver);
}

static void remote_set_speed(float rpm)
{
    rpm = remote_clamp(rpm, -REMOTE_RPM_LIMIT, REMOTE_RPM_LIMIT);
    g_motor.mode = MOTOR_MODE_VELOCITY;
    requested_speed = rpm * TWO_PI / 60.0f;
    remote_mode = REMOTE_MODE_SPEED;
    remote_enabled = 1;
    last_heartbeat_ms = g_tick_ms;
    Driver_Enable(&g_driver);
}

static void remote_set_position(float degrees)
{
    degrees = remote_clamp(degrees, -REMOTE_POSITION_LIMIT,
                           REMOTE_POSITION_LIMIT);
    g_motor.mode = MOTOR_MODE_ANGLE;
    g_motor.target_angle = degrees * 3.14159265359f / 180.0f;
    g_motor.target_velocity = 0.0f;
    remote_mode = REMOTE_MODE_POSITION;
    remote_enabled = 1;
    last_heartbeat_ms = g_tick_ms;
    Driver_Enable(&g_driver);
}

static void remote_set_openloop_speed(float rpm)
{
    float previous_requested = requested_speed;
    float new_requested;
    uint8_t needs_resync;

    rpm = remote_clamp(rpm, -REMOTE_RPM_LIMIT, REMOTE_RPM_LIMIT);
    new_requested = rpm * TWO_PI / 60.0f;
    needs_resync = (remote_mode != REMOTE_MODE_OPENLOOP);

    /* 高速失步后降低目标转速时，虚拟电角度已不可信，需要重新同步 */
    if (remote_mode == REMOTE_MODE_OPENLOOP &&
        fabsf(new_requested) < fabsf(previous_requested) * 0.9f &&
        fabsf(g_motor.shaft_velocity) < fabsf(new_requested) * 0.9f)
    {
        needs_resync = 1;
    }

    if (needs_resync)
    {
        open_loop_electrical_angle = FOC_ElectricalAngle(&g_motor);
        open_loop_start_tick = g_tick_ms;
        open_loop_start_angle = g_motor.shaft_angle;
        open_loop_started = 0;
        if (remote_mode == REMOTE_MODE_OPENLOOP)
            printf("EVT,OPENLOOP_RESYNC\r\n");
    }

    requested_speed = new_requested;
    g_motor.mode = MOTOR_MODE_VELOCITY;
    remote_mode = REMOTE_MODE_OPENLOOP;
    remote_enabled = 1;
    last_heartbeat_ms = g_tick_ms;
    Driver_Enable(&g_driver);
}

static void remote_set_torque_speed(float rpm)
{
    rpm = remote_clamp(rpm, -REMOTE_RPM_LIMIT, REMOTE_RPM_LIMIT);
    if (remote_mode != REMOTE_MODE_TORQUE)
    {
        slow_speed = g_motor.shaft_velocity;
        speed_trim = 0.0f;
    }

    requested_speed = rpm * TWO_PI / 60.0f;
    g_motor.mode = MOTOR_MODE_VELOCITY;
    remote_mode = REMOTE_MODE_TORQUE;
    remote_enabled = 1;
    last_heartbeat_ms = g_tick_ms;
    Driver_Enable(&g_driver);
}

static void remote_command(char *line)
{
    char *value;

    while (*line == ' ') line++;

    if (strcmp(line, "STOP") == 0)
    {
        remote_stop();
    }
    else if (strcmp(line, "ZERO") == 0)
    {
        remote_stop();
        g_motor.shaft_angle = 0.0f;
        g_motor.target_angle = 0.0f;
    }
    else if (strcmp(line, "START") == 0 || strcmp(line, "EN") == 0)
    {
        remote_start();
    }
    else if (strcmp(line, "K") == 0)
    {
        last_heartbeat_ms = g_tick_ms;
    }
    else if ((line[0] == 'V' || line[0] == 'P') &&
             (line[1] == ',' || line[1] == ' '))
    {
        value = line + 2;
        while (*value == ' ') value++;

        if (*value != '\0')
        {
            float number = (float)atof(value);
            if (line[0] == 'V')
                remote_set_openloop_speed(number);
            else
                remote_set_position(number);
        }
    }
    else if (line[0] == 'F' && (line[1] == ',' || line[1] == ' '))
    {
        value = line + 2;
        while (*value == ' ') value++;
        if (*value != '\0')
            remote_set_speed((float)atof(value));
    }
    else if (line[0] == 'S' && (line[1] == ',' || line[1] == ' '))
    {
        value = line + 2;
        while (*value == ' ') value++;
        if (*value != '\0')
            remote_set_torque_speed((float)atof(value));
    }
    else if (line[0] == 'O' && (line[1] == ',' || line[1] == ' '))
    {
        value = line + 2;
        while (*value == ' ') value++;
        if (*value != '\0')
            remote_set_openloop_speed((float)atof(value));
    }
}

static void remote_process_serial(void)
{
    uint8_t ch;

    while (Serial_ReadByte(&ch))
    {
        if (ch == '\n' || ch == '\r')
        {
            if (command_length > 0)
            {
                command_line[command_length] = '\0';
                remote_command(command_line);
                command_length = 0;
            }
        }
        else if (command_length < sizeof(command_line) - 1U)
        {
            command_line[command_length++] = (char)ch;
        }
        else
        {
            command_length = 0;
        }
    }

    if (remote_enabled &&
        (g_tick_ms - last_heartbeat_ms > REMOTE_HEARTBEAT_MS))
    {
        remote_stop();
        printf("EVT,HEARTBEAT_TIMEOUT\r\n");
    }
}

static void remote_ramp_speed(float Ts)
{
    float max_step;
    float difference;

    if (!remote_enabled ||
        (remote_mode != REMOTE_MODE_SPEED &&
         remote_mode != REMOTE_MODE_OPENLOOP &&
         remote_mode != REMOTE_MODE_TORQUE))
        return;

    max_step = SPEED_COMMAND_RAMP * Ts;
    difference = requested_speed - g_motor.target_velocity;
    if (difference > max_step)
        g_motor.target_velocity += max_step;
    else if (difference < -max_step)
        g_motor.target_velocity -= max_step;
    else
        g_motor.target_velocity = requested_speed;
}

static void remote_send_telemetry(void)
{
    uint32_t elapsed;

    if (g_tick_ms - last_telemetry_ms < TELEMETRY_PERIOD_MS)
        return;

    last_telemetry_ms = g_tick_ms;
    elapsed = g_tick_ms - telemetry_speed_tick;
    if (elapsed > 0)
    {
        telemetry_speed_rpm =
            (g_motor.shaft_angle - telemetry_speed_angle) /
            ((float)elapsed * 0.001f) * 60.0f / TWO_PI;
        telemetry_speed_tick = g_tick_ms;
        telemetry_speed_angle = g_motor.shaft_angle;
    }

    printf("FOC,%lu,%u,%u,%.4f,%.4f,%.2f,%.2f,%.4f\r\n",
           (unsigned long)g_tick_ms,
           (unsigned int)remote_enabled,
           (unsigned int)remote_mode,
           g_motor.target_velocity * 60.0f / TWO_PI,
           telemetry_speed_rpm,
           g_motor.target_angle * 180.0f / 3.14159265359f,
           g_motor.shaft_angle * 180.0f / 3.14159265359f,
           g_motor.Uq);
}
#endif

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
    Delay_ms(100);
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
    g_motor.P_velocity.P = SPEED_KP;
    g_motor.P_velocity.I = SPEED_KI;
    g_motor.torque_sign = MOTOR_TORQUE_SIGN;
    g_motor.P_angle.P = 5.0f;
    g_motor.P_angle.limit = 5.0f;
    g_motor.P_angle.output_ramp = 20.0f;

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

    /* 对齐完成后保持停止，等待上位机命令 */
    g_motor.mode = MOTOR_MODE_VELOCITY;
    remote_stop();
    printf("READY: V,<rpm> | P,<deg> | START | STOP | K\r\n");

    Tick_Init();
    {
        uint32_t stall_start = 0;
        uint32_t sync_loss_start = 0;
        uint32_t motion_check_tick;
        uint32_t last_control_tick;
        float motion_check_angle;
        float control_ts;
        uint8_t stalled = 0;
        uint8_t sync_lost = 0;

        last_heartbeat_ms = g_tick_ms;
        last_telemetry_ms = g_tick_ms;
        telemetry_speed_tick = g_tick_ms;
        telemetry_speed_angle = g_motor.shaft_angle;
        motion_check_tick = g_tick_ms;
        motion_check_angle = g_motor.shaft_angle;
        last_control_tick = g_tick_ms;

        while (1)
        {
            if (g_foc_tick)
            {
                uint32_t now_tick = g_tick_ms;
                uint32_t elapsed_ticks = now_tick - last_control_tick;

                g_foc_tick = 0;
                if (elapsed_ticks == 0)
                    elapsed_ticks = 1;
                if (elapsed_ticks > 10U)
                    elapsed_ticks = 10U;
                control_ts = (float)elapsed_ticks * 0.001f;
                last_control_tick = now_tick;

                if (remote_enabled)
                {
                    remote_ramp_speed(control_ts);
                    if (remote_mode == REMOTE_MODE_OPENLOOP)
                    {
                        if (FOC_UpdateSensor(&g_motor, control_ts) == 0)
                        {
                            float rpm;
                            float uq;
                            float startup_boost;
                            uint32_t startup_elapsed;

                            rpm = fabsf(g_motor.target_velocity) *
                                  60.0f / TWO_PI;
                            uq = (rpm > 0.05f)
                                ? OPENLOOP_UQ_BASE +
                                  OPENLOOP_UQ_PER_RPM * rpm
                                : 0.0f;
                            startup_elapsed = g_tick_ms - open_loop_start_tick;
                            if (!open_loop_started)
                            {
                                if (startup_elapsed < OPENLOOP_RAMP_MS)
                                    startup_boost = OPENLOOP_START_UQ *
                                        ((float)startup_elapsed /
                                         (float)OPENLOOP_RAMP_MS);
                                else
                                    startup_boost = OPENLOOP_START_UQ;
                                open_loop_electrical_angle =
                                    FOC_ElectricalAngle(&g_motor);

                                if (fabsf(g_motor.shaft_angle -
                                          open_loop_start_angle) > 0.03f)
                                    open_loop_started = 1;
                            }
                            else
                            {
                                startup_boost = 0.0f;
                                open_loop_electrical_angle +=
                                    g_motor.target_velocity *
                                    g_motor.pole_pairs * control_ts;
                                if (open_loop_electrical_angle >= TWO_PI)
                                    open_loop_electrical_angle -= TWO_PI;
                                else if (open_loop_electrical_angle < 0.0f)
                                    open_loop_electrical_angle += TWO_PI;
                            }
                            if (uq < startup_boost)
                                uq = startup_boost;
                            uq = remote_clamp(
                                uq, 0.0f, MAX_PHASE_VOLTAGE);
                            g_motor.Uq = uq;
                            FOC_SetPhaseVoltage(
                                &g_motor,
                                g_motor.torque_sign * uq,
                                0.0f,
                                open_loop_electrical_angle);
                        }
                    }
                    else if (remote_mode == REMOTE_MODE_TORQUE)
                    {
                        if (FOC_UpdateSensor(&g_motor, control_ts) == 0)
                        {
                            float rpm;
                            float feedforward;
                            float error;
                            float uq;

                            slow_speed +=
                                control_ts / (0.25f + control_ts) *
                                (g_motor.shaft_velocity - slow_speed);
                            error = g_motor.target_velocity - slow_speed;
                            speed_trim += 0.08f * error * control_ts;
                            speed_trim = remote_clamp(
                                speed_trim, -0.35f, 0.35f);

                            rpm = fabsf(g_motor.target_velocity) *
                                  60.0f / TWO_PI;
                            feedforward = (rpm > 0.05f)
                                ? OPENLOOP_UQ_BASE +
                                  OPENLOOP_UQ_PER_RPM * rpm
                                : 0.0f;
                            uq = feedforward + 0.08f * error + speed_trim;
                            uq = remote_clamp(uq, 0.0f, MAX_PHASE_VOLTAGE);
                            g_motor.Uq = uq;
                            FOC_SetPhaseVoltage(
                                &g_motor,
                                g_motor.torque_sign * uq,
                                0.0f,
                                FOC_ElectricalAngle(&g_motor));
                        }
                    }
                    else
                    {
                        FOC_Loop(&g_motor, control_ts);
                    }
                }
                else
                    FOC_UpdateSensor(&g_motor, control_ts);

                if (!g_motor.aligned && remote_enabled)
                    remote_stop();

                remote_process_serial();

                if (remote_enabled &&
                    fabsf(g_motor.target_velocity) > 0.2f &&
                    fabsf(g_motor.shaft_velocity) < 0.1f &&
                    fabsf(g_motor.Uq) >= MAX_PHASE_VOLTAGE * 0.9f)
                {
                    if (!stalled)
                    {
                        stalled = 1;
                        stall_start = g_tick_ms;
                    }
                    else if (g_tick_ms - stall_start >= STALL_TIMEOUT_MS)
                    {
                        remote_stop();
                        printf("STALL PROTECTION: Uq=%.2f V, speed=%.3f rad/s\r\n",
                               g_motor.Uq, g_motor.shaft_velocity);
                    }
                }
                else
                {
                    stalled = 0;
                }

                if (remote_enabled &&
                    (remote_mode == REMOTE_MODE_OPENLOOP ||
                     remote_mode == REMOTE_MODE_TORQUE) &&
                    fabsf(g_motor.target_velocity) > 0.2f)
                {
                    if (remote_mode == REMOTE_MODE_OPENLOOP &&
                        !open_loop_started)
                    {
                        if (now_tick - open_loop_start_tick >
                            OPENLOOP_START_TIMEOUT)
                        {
                            remote_stop();
                            printf("STALL PROTECTION: open-loop start timeout\r\n");
                        }
                        motion_check_tick = now_tick;
                        motion_check_angle = g_motor.shaft_angle;
                    }
                    else if (now_tick - motion_check_tick >= 1000U)
                    {
                        float moved = fabsf(
                            g_motor.shaft_angle - motion_check_angle);
                        if (moved < 0.05f)
                        {
                            remote_stop();
                            printf("STALL PROTECTION: no mechanical movement\r\n");
                        }
                        motion_check_tick = now_tick;
                        motion_check_angle = g_motor.shaft_angle;
                    }
                }
                else
                {
                    motion_check_tick = now_tick;
                    motion_check_angle = g_motor.shaft_angle;
                }

                if (remote_enabled &&
                    remote_mode == REMOTE_MODE_OPENLOOP &&
                    fabsf(g_motor.target_velocity) > 1.0f &&
                    fabsf(telemetry_speed_rpm) <
                        fabsf(g_motor.target_velocity) * 60.0f /
                        TWO_PI * 0.65f)
                {
                    if (!sync_lost)
                    {
                        sync_lost = 1;
                        sync_loss_start = now_tick;
                    }
                    else if (now_tick - sync_loss_start > 1500U)
                    {
                        remote_stop();
                        printf("STALL PROTECTION: V/f synchronism lost\r\n");
                    }
                }
                else
                {
                    sync_lost = 0;
                }

                remote_send_telemetry();
            }
        }
    }
#endif
#endif
}
