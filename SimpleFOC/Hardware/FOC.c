#include "FOC.h"
#include "Delay.h"
#include <math.h>
#include <stdio.h>

#define _2PI       6.28318530718f
#define _PI        3.14159265359f
#define _3PI_2     4.71238898038f
#define _SQRT3_2   0.86602540378f
#define _CONSTRAIN(amt, low, high)  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

volatile float g_align_start_angle = -1.0f;
volatile float g_align_end_angle = -1.0f;
volatile float g_align_delta = 0.0f;

/* 角度归一化到 0~2pi */
static float normalize_angle(float a)
{
    a = fmodf(a, _2PI);
    if (a < 0.0f) a += _2PI;
    return a;
}

static float read_mechanical_angle_retry(uint8_t attempts)
{
    uint8_t i;

    for (i = 0; i < attempts; i++)
    {
        float angle = AS5600_GetMechAngle();
        if (angle >= 0.0f)
            return angle;
        Delay_ms(2);
    }
    return -1.0f;
}

void FOC_Init(FOCMotor *m, uint8_t pp, float Vpower)
{
    m->driver = 0;
    m->pole_pairs = pp;
    m->voltage_power_supply = Vpower;
    m->voltage_limit = Vpower / 2.0f;
    m->voltage_sensor_align = 3.0f;        /* 云台电机对齐电压，力量小、安全 */
    m->sensor_direction = 1.0f;
    m->torque_sign = 1.0f;
    m->zero_electric_angle = 0.0f;
    m->aligned = 0;
    m->mode = MOTOR_MODE_ANGLE;
    m->target_angle = 0.0f;
    m->target_velocity = 0.0f;
    m->Uq = 0.0f;
    m->shaft_angle = 0.0f;
    m->shaft_velocity = 0.0f;
    m->lpf_time = 0.02f;
    m->speed_window = 0.02f;
    m->speed_delta_sum = 0.0f;
    m->speed_time_sum = 0.0f;
    m->speed_index = 0;
    m->speed_count = 0;
    m->mech_prev = read_mechanical_angle_retry(5);
    m->mech_angle = m->mech_prev;
    if (m->mech_prev < 0.0f)
    {
        m->mech_prev = 0.0f;
        m->mech_angle = 0.0f;
    }
    m->vel_prev = 0.0f;

    /* 角度环：输出目标速度(rad/s)，限 10 rad/s */
    PID_Init(&m->P_angle, 10.0f, 0.0f, 0.0f, 1000.0f, 10.0f);
    /* 速度环：输出 q 轴电压(V)，限 voltage_limit */
    PID_Init(&m->P_velocity, 0.6f, 5.0f, 0.0f, 1000.0f, m->voltage_limit);
}

void FOC_SetPhaseVoltage(FOCMotor *m, float Uq, float Ud, float angle_el)
{
    float Ua, Ub, Uc, Ualpha, Ubeta, center;
    float ca = cosf(angle_el);
    float sa = sinf(angle_el);

    /* 逆帕克 */
    Ualpha = ca * Ud - sa * Uq;
    Ubeta  = sa * Ud + ca * Uq;
    /* 逆克拉克（正弦调制） */
    Ua = Ualpha;
    Ub = -0.5f * Ualpha + _SQRT3_2 * Ubeta;
    Uc = -0.5f * Ualpha - _SQRT3_2 * Ubeta;

    /* 中心偏移：把三相抬到电源中点，使占空比落在 0~1 */
    center = m->voltage_power_supply / 2.0f;
    Ua += center;
    Ub += center;
    Uc += center;

    Driver_SetPwm(m->driver, Ua, Ub, Uc);
}

float FOC_ElectricalAngle(FOCMotor *m)
{
    return normalize_angle(m->sensor_direction * m->mech_angle * m->pole_pairs -
                           m->zero_electric_angle);
}

/* 更新多圈机械角度与速度（低通） */
static uint8_t update_sensor(FOCMotor *m, float Ts)
{
    float mech = read_mechanical_angle_retry(3);
    float corrected_delta;
    float d;
    float max_delta;

    if (mech < 0.0f)
        return 1;

    d = normalize_angle(mech - m->mech_prev);
    if (d > _PI) d -= _2PI;              /* 归一化到 -pi~pi */

    /* 软件 I2C 偶发误码会产生不可信的机械角跳变，二次读取确认 */
    max_delta = 10.0f * Ts + 0.004f;
    if (fabsf(d) > max_delta)
    {
        float retry = AS5600_GetMechAngle();
        if (retry < 0.0f)
            return 0;                    /* 丢弃本周期样本，不触发停机 */

        mech = retry;
        d = normalize_angle(mech - m->mech_prev);
        if (d > _PI) d -= _2PI;
        if (fabsf(d) > max_delta)
            return 0;
    }

    corrected_delta = m->sensor_direction * d;
    m->shaft_angle += corrected_delta;
    m->mech_angle = mech;

    if (m->speed_count == FOC_SPEED_HISTORY)
    {
        m->speed_delta_sum -= m->speed_history_delta[m->speed_index];
        m->speed_time_sum -= m->speed_history_time[m->speed_index];
    }
    else
    {
        m->speed_count++;
    }

    m->speed_history_delta[m->speed_index] = corrected_delta;
    m->speed_history_time[m->speed_index] = Ts;
    m->speed_delta_sum += corrected_delta;
    m->speed_time_sum += Ts;
    m->speed_index = (m->speed_index + 1U) % FOC_SPEED_HISTORY;

    while (m->speed_count > 1U)
    {
        uint8_t oldest = (m->speed_index + FOC_SPEED_HISTORY -
                          m->speed_count) % FOC_SPEED_HISTORY;
        if (m->speed_time_sum - m->speed_history_time[oldest] <
            m->speed_window)
            break;

        m->speed_delta_sum -= m->speed_history_delta[oldest];
        m->speed_time_sum -= m->speed_history_time[oldest];
        m->speed_count--;
    }

    if (m->speed_time_sum > 0.0f)
    {
        float v = m->speed_delta_sum / m->speed_time_sum;
        float a = m->lpf_time / (m->lpf_time + Ts);
        m->shaft_velocity = a * m->vel_prev + (1.0f - a) * v;
        m->vel_prev = m->shaft_velocity;
    }
    m->mech_prev = mech;
    return 0;
}

uint8_t FOC_UpdateSensor(FOCMotor *m, float Ts)
{
    if (!m->aligned)
        return 1;

    if (update_sensor(m, Ts) != 0)
    {
        m->Uq = 0.0f;
        m->aligned = 0;
        Driver_Disable(m->driver);
        return 1;
    }
    return 0;
}

uint8_t FOC_Align(FOCMotor *m)
{
    float mech_start, mech_end, dm, mech;
    uint8_t st;
    int i;

    /* 1. 检查 AS5600 磁铁状态：bit5 MD 必须为1 */
    st = AS5600_GetStatus();
    if (st == 0xFF || !(st & 0x20))
        return 1;     /* 未检测到磁铁（或I2C异常） */

    /* 2. 在电角度0施加电压，锁定转子 */
    /* Uq 在 3π/2 时对应 alpha 轴，转子 d 轴与电压矢量对齐 */
    FOC_SetPhaseVoltage(m, m->voltage_sensor_align, 0.0f, _3PI_2);
    Delay_ms(1200);

    /* 3. 缓慢让电角度正向增加，检测传感器方向 */
    mech_start = read_mechanical_angle_retry(5);
    g_align_start_angle = mech_start;
    if (mech_start < 0.0f)
    {
        FOC_SetPhaseVoltage(m, 0.0f, 0.0f, 0.0f);
        Driver_Disable(m->driver);
        printf("Align sensor error: start read failed\r\n");
        return 2;
    }

    for (i = 1; i <= 7; i++)
    {
        FOC_SetPhaseVoltage(m, m->voltage_sensor_align, 0.0f,
                            _3PI_2 + i * 0.15f);
        Delay_ms(100);
    }
    mech_end = read_mechanical_angle_retry(5);
    g_align_end_angle = mech_end;
    if (mech_end < 0.0f)
    {
        FOC_SetPhaseVoltage(m, 0.0f, 0.0f, 0.0f);
        Driver_Disable(m->driver);
        printf("Align sensor error: end read failed\r\n");
        return 3;
    }
    dm = normalize_angle(mech_end - mech_start);
    if (dm > _PI) dm -= _2PI;
    g_align_delta = dm;
    printf("Align travel: start=%.3f end=%.3f dm=%.3f\r\n",
           mech_start, mech_end, dm);
    if (fabsf(dm) < 0.01f)
    {
        FOC_SetPhaseVoltage(m, 0.0f, 0.0f, 0.0f);
        Driver_Disable(m->driver);
        return 4;     /* 开环施加电压后转子没有移动 */
    }
    m->sensor_direction = (dm > 0.0f) ? 1.0f : -1.0f;

    /* 4. 重新锁定到电角度0，记录零位 */
    FOC_SetPhaseVoltage(m, m->voltage_sensor_align, 0.0f, _3PI_2);
    Delay_ms(1200);
    mech = read_mechanical_angle_retry(5);
    if (mech < 0.0f)
    {
        FOC_SetPhaseVoltage(m, 0.0f, 0.0f, 0.0f);
        Driver_Disable(m->driver);
        return 5;
    }
    m->zero_electric_angle = m->sensor_direction * mech * m->pole_pairs;

    /* 5. 释放电压，复位累计量，进入闭环 */
    FOC_SetPhaseVoltage(m, 0.0f, 0.0f, 0.0f);
    Delay_ms(200);

    m->mech_prev = read_mechanical_angle_retry(5);
    m->mech_angle = m->mech_prev;
    m->shaft_angle = 0.0f;
    m->vel_prev = 0.0f;
    m->shaft_velocity = 0.0f;
    m->speed_delta_sum = 0.0f;
    m->speed_time_sum = 0.0f;
    m->speed_index = 0;
    m->speed_count = 0;
    PID_Reset(&m->P_angle);
    PID_Reset(&m->P_velocity);
    m->aligned = 1;
    return 0;
}

void FOC_Loop(FOCMotor *m, float Ts)
{
    float angle_el;

    if (!m->aligned) return;

    if (update_sensor(m, Ts) != 0)
    {
        m->Uq = 0.0f;
        m->aligned = 0;
        Driver_Disable(m->driver);
        return;
    }

    if (m->mode == MOTOR_MODE_ANGLE)
    {
        float angle_err = m->target_angle - m->shaft_angle;
        m->target_velocity = PID_Operator(&m->P_angle, angle_err, Ts);
        {
            float vel_err = m->target_velocity - m->shaft_velocity;
            m->Uq = PID_Operator(&m->P_velocity, vel_err, Ts);
        }
    }
    else
    {
        float vel_err = m->target_velocity - m->shaft_velocity;
        m->Uq = PID_Operator(&m->P_velocity, vel_err, Ts);
    }

    angle_el = FOC_ElectricalAngle(m);
    if (angle_el < 0.0f)
    {
        m->Uq = 0.0f;
        m->aligned = 0;
        Driver_Disable(m->driver);
        return;
    }
    FOC_SetPhaseVoltage(m, m->torque_sign * m->Uq, 0.0f, angle_el);
}
