#ifndef __ATK_MS901M_H
#define __ATK_MS901M_H

/*
 * ATK-MS901M (ATK-IMU901) 主动上报帧解析（与硬件无关，逐字节喂入）
 *
 * 帧格式： 0x55 0x55 | ID | LEN | DATA[LEN] (小端) | SUM
 * SUM = (0x55 + 0x55 + ID + LEN + 全部数据字节) & 0xFF
 * 帧总长 = 5 + LEN
 */

#include <stdint.h>

/*
 * 模块类型选择：
 *   1 = ATK-MS601M 六轴（默认，仅姿态/四元数/加速度/陀螺仪；
 *       0x04 为端口状态、无 0x05 气压；加速度默认量程 4g）
 *   0 = ATK-MS901M 十轴（额外有磁力计 0x04、气压计 0x05；加速度默认 2g）
 * 两种模块的帧头/校验/0x01/0x02/0x03 完全一致，本解析器按下面的宏做区分。
 */
#ifndef ATK_MODULE_MS601M
#define ATK_MODULE_MS601M 1
#endif

/* 量程（模块出厂默认；若用厂商上位机改过，需同步修改） */
#define ATK_MS901M_GYRO_FSR_DPS    2000.0f    /* 陀螺仪满量程 ±2000 °/s（两模块默认一致） */
#if ATK_MODULE_MS601M
#define ATK_MS901M_ACC_FSR_G       4.0f       /* MS601M 加速度默认满量程 ±4 g */
#else
#define ATK_MS901M_ACC_FSR_G       2.0f       /* MS901M 加速度默认满量程 ±2 g */
#endif

/* 帧 ID */
#define ATK_MS901M_ID_ATTITUDE     0x01       /* 姿态角 roll/pitch/yaw */
#define ATK_MS901M_ID_QUATERNION   0x02       /* 四元数 q0~q3 */
#define ATK_MS901M_ID_ACC_GYRO     0x03       /* 加速度 + 陀螺仪 */
#define ATK_MS901M_ID_MAG          0x04       /* MS901M: 磁力计+温度；MS601M: 端口状态(忽略) */
#define ATK_MS901M_ID_BARO         0x05       /* MS901M: 气压+海拔+温度；MS601M 无此帧 */

/* 解析完成标志位（atk_ms901m_parse_byte 返回值） */
#define ATK_MS901M_VALID_ATTITUDE     0x01
#define ATK_MS901M_VALID_QUATERNION   0x02
#define ATK_MS901M_VALID_ACC_GYRO     0x04
#define ATK_MS901M_VALID_MAG          0x08
#define ATK_MS901M_VALID_BARO         0x10

typedef struct
{
    float roll;
    float pitch;
    float yaw;
} atk_ms901m_attitude_t;

typedef struct
{
    float q[4];
} atk_ms901m_quaternion_t;

typedef struct
{
    float x;
    float y;
    float z;
} atk_ms901m_axis3_t;

typedef struct
{
    int16_t x;
    int16_t y;
    int16_t z;
    float temperature;
} atk_ms901m_mag_t;

typedef struct
{
    int32_t pressure;      /* 气压，Pa */
    int32_t altitude;      /* 海拔，cm */
    float temperature;     /* 温度，°C */
} atk_ms901m_baro_t;

typedef struct
{
    atk_ms901m_attitude_t   attitude;
    atk_ms901m_quaternion_t quat;
    atk_ms901m_axis3_t      gyro;
    atk_ms901m_axis3_t      acc;
    atk_ms901m_mag_t        mag;
    atk_ms901m_baro_t       baro;

    volatile uint32_t frame_count;     /* 校验正确的帧计数 */
    volatile uint32_t crc_error;       /* 校验错误帧计数 */
} atk_ms901m_data_t;

/* 初始化（清零） */
void atk_ms901m_init(atk_ms901m_data_t *data);

/*
 * 喂入一个字节，内部完成状态机解析。
 * 返回值：本次字节恰好完成某帧时返回对应 ATK_MS901M_VALID_xxx 标志，否则返回 0。
 */
uint8_t atk_ms901m_parse_byte(atk_ms901m_data_t *data, uint8_t byte);

#endif
