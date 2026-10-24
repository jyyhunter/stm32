#include "atk_ms901m.h"

#define FRAME_HEADER_L    0x55
#define FRAME_HEADER_H    0x55
#define FRAME_MAX_LEN     16

enum
{
    PARSE_STATE_HEAD_L = 0,
    PARSE_STATE_HEAD_H,
    PARSE_STATE_ID,
    PARSE_STATE_LEN,
    PARSE_STATE_DATA,
    PARSE_STATE_SUM
};

static int16_t rd_i16(const uint8_t *p)
{
    return (int16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

#if !ATK_MODULE_MS601M
static int32_t rd_i32(const uint8_t *p)
{
    return (int32_t)((uint32_t)p[0] |
                     ((uint32_t)p[1] << 8) |
                     ((uint32_t)p[2] << 16) |
                     ((uint32_t)p[3] << 24));
}
#endif

void atk_ms901m_init(atk_ms901m_data_t *data)
{
    uint8_t *p = (uint8_t *)data;
    uint32_t i;
    for (i = 0; i < sizeof(atk_ms901m_data_t); i++)
        p[i] = 0;
}

/* 解析一帧数据体，返回该帧的有效标志；校验/长度异常返回 0 */
static uint8_t atk_ms901m_decode(atk_ms901m_data_t *data, uint8_t id,
                                 const uint8_t *dat, uint8_t len)
{
#if !ATK_MODULE_MS601M
    int16_t raw;
#endif
    switch (id)
    {
    case ATK_MS901M_ID_ATTITUDE:
        if (len < 6)
            return 0;
        data->attitude.roll  = (float)rd_i16(&dat[0]) / 32768.0f * 180.0f;
        data->attitude.pitch = (float)rd_i16(&dat[2]) / 32768.0f * 180.0f;
        data->attitude.yaw   = (float)rd_i16(&dat[4]) / 32768.0f * 180.0f;
        return ATK_MS901M_VALID_ATTITUDE;

    case ATK_MS901M_ID_QUATERNION:
        if (len < 8)
            return 0;
        data->quat.q[0] = (float)rd_i16(&dat[0]) / 32768.0f;
        data->quat.q[1] = (float)rd_i16(&dat[2]) / 32768.0f;
        data->quat.q[2] = (float)rd_i16(&dat[4]) / 32768.0f;
        data->quat.q[3] = (float)rd_i16(&dat[6]) / 32768.0f;
        return ATK_MS901M_VALID_QUATERNION;

    case ATK_MS901M_ID_ACC_GYRO:
        if (len < 12)
            return 0;
        data->acc.x = (float)rd_i16(&dat[0]) / 32768.0f * ATK_MS901M_ACC_FSR_G;
        data->acc.y = (float)rd_i16(&dat[2]) / 32768.0f * ATK_MS901M_ACC_FSR_G;
        data->acc.z = (float)rd_i16(&dat[4]) / 32768.0f * ATK_MS901M_ACC_FSR_G;
        data->gyro.x = (float)rd_i16(&dat[6]) / 32768.0f * ATK_MS901M_GYRO_FSR_DPS;
        data->gyro.y = (float)rd_i16(&dat[8]) / 32768.0f * ATK_MS901M_GYRO_FSR_DPS;
        data->gyro.z = (float)rd_i16(&dat[10]) / 32768.0f * ATK_MS901M_GYRO_FSR_DPS;
        return ATK_MS901M_VALID_ACC_GYRO;

#if !ATK_MODULE_MS601M
    /* 仅十轴 MS901M：0x04 磁力计+温度、0x05 气压+海拔+温度。
       六轴 MS601M 的 0x04 为端口状态(D0~D3)、无 0x05，默认忽略。 */
    case ATK_MS901M_ID_MAG:
        if (len < 8)
            return 0;
        data->mag.x = rd_i16(&dat[0]);
        data->mag.y = rd_i16(&dat[2]);
        data->mag.z = rd_i16(&dat[4]);
        raw = rd_i16(&dat[6]);
        data->mag.temperature = (float)raw / 100.0f;
        return ATK_MS901M_VALID_MAG;

    case ATK_MS901M_ID_BARO:
        if (len < 10)
            return 0;
        data->baro.pressure = rd_i32(&dat[0]);
        data->baro.altitude = rd_i32(&dat[4]);
        raw = rd_i16(&dat[8]);
        data->baro.temperature = (float)raw / 100.0f;
        return ATK_MS901M_VALID_BARO;
#endif /* !ATK_MODULE_MS601M */

    default:
        return 0;
    }
}

uint8_t atk_ms901m_parse_byte(atk_ms901m_data_t *data, uint8_t byte)
{
    static uint8_t state = PARSE_STATE_HEAD_L;
    static uint8_t id;
    static uint8_t len;
    static uint8_t payload[FRAME_MAX_LEN];
    static uint8_t index;
    static uint8_t sum;
    uint8_t valid = 0;

    switch (state)
    {
    case PARSE_STATE_HEAD_L:
        if (byte == FRAME_HEADER_L)
            state = PARSE_STATE_HEAD_H;
        break;

    case PARSE_STATE_HEAD_H:
        if (byte == FRAME_HEADER_H)
            state = PARSE_STATE_ID;
        else if (byte != FRAME_HEADER_L)
            state = PARSE_STATE_HEAD_L;
        break;

    case PARSE_STATE_ID:
        id = byte;
        sum = (uint8_t)(FRAME_HEADER_L + FRAME_HEADER_H + id);
        state = PARSE_STATE_LEN;
        break;

    case PARSE_STATE_LEN:
        len = byte;
        sum = (uint8_t)(sum + len);
        index = 0;
        if ((len == 0) || (len > FRAME_MAX_LEN))
            state = PARSE_STATE_HEAD_L;
        else
            state = PARSE_STATE_DATA;
        break;

    case PARSE_STATE_DATA:
        payload[index] = byte;
        sum = (uint8_t)(sum + byte);
        index++;
        if (index >= len)
            state = PARSE_STATE_SUM;
        break;

    case PARSE_STATE_SUM:
        if (byte == sum)
        {
            valid = atk_ms901m_decode(data, id, payload, len);
            if (valid)
                data->frame_count++;
            /* 校验正确但 ID 未知：忽略，不计错误 */
        }
        else
        {
            data->crc_error++;
        }
        state = PARSE_STATE_HEAD_L;
        break;

    default:
        state = PARSE_STATE_HEAD_L;
        break;
    }
    return valid;
}
