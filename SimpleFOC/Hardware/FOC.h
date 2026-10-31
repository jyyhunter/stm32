#ifndef __FOC_H
#define __FOC_H

#include "stm32f10x.h"
#include "BLDCDriver.h"
#include "AS5600.h"
#include "PID.h"

#define FOC_SPEED_HISTORY 32U

/* 控制模式 */
typedef enum {
    MOTOR_MODE_ANGLE = 0,    /* 角度闭环（位置） */
    MOTOR_MODE_VELOCITY = 1  /* 速度闭环 */
} FOCMode;

typedef struct {
    /* ---- 电机参数 ---- */
    uint8_t pole_pairs;            /* 极对数 */
    float zero_electric_angle;     /* 传感器零位电角度 */
    float sensor_direction;        /* 传感器方向，对齐时自动检测：1 或 -1 */
    float torque_sign;             /* 转矩方向修正：1 或 -1 */

    /* ---- 驱动 ---- */
    BLDCDriver *driver;

    /* ---- 电压 ---- */
    float voltage_power_supply;    /* 电源电压 */
    float voltage_limit;           /* 电机最大电压（一般取电源/2） */
    float voltage_sensor_align;    /* 对齐时施加的电压 */

    /* ---- 控制器 ---- */
    PIDController P_angle;         /* 角度环，输出目标速度 */
    PIDController P_velocity;      /* 速度环，输出 Uq */

    /* ---- 目标 ---- */
    float target_angle;
    float target_velocity;

    /* ---- 运行状态 ---- */
    float shaft_angle;             /* 累计机械角度(rad)，多圈 */
    float shaft_velocity;          /* 机械角速度(rad/s)，低通后 */
    float Uq;                      /* 当前 q 轴电压 */

    /* ---- 传感器内部 ---- */
    float mech_prev;
    float mech_angle;
    float vel_prev;
    float lpf_time;                /* 速度低通时间常数(s) */
    float speed_window;            /* 速度窗口采样周期(s) */
    float speed_history_delta[FOC_SPEED_HISTORY];
    float speed_history_time[FOC_SPEED_HISTORY];
    float speed_delta_sum;
    float speed_time_sum;
    uint8_t speed_index;
    uint8_t speed_count;

    FOCMode mode;
    uint8_t aligned;
} FOCMotor;

extern volatile float g_align_start_angle;
extern volatile float g_align_end_angle;
extern volatile float g_align_delta;

/* 初始化电机：pp=极对数，Vpower=电源电压 */
void FOC_Init(FOCMotor *m, uint8_t pp, float Vpower);

/* 传感器/零位自动对齐（含方向检测），0=成功，非0=失败 */
uint8_t FOC_Align(FOCMotor *m);

/* 每个控制周期调用，Ts=周期(s)，完成角度/速度闭环 */
void FOC_Loop(FOCMotor *m, float Ts);

/* 仅更新传感器与多圈位置，不计算或输出转矩 */
uint8_t FOC_UpdateSensor(FOCMotor *m, float Ts);

/* 直接给定电角度施加电压（对齐/开环用） */
void FOC_SetPhaseVoltage(FOCMotor *m, float Uq, float Ud, float angle_el);

/* 计算当前电角度 */
float FOC_ElectricalAngle(FOCMotor *m);

#endif
