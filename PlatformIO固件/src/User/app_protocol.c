#include "app_protocol.h"
#include "Serial.h"
#include <stdio.h>

#define FRAME_CAPACITY 160



/* 定点浮点转字符串（不使用 %f，兼容 MicroLib） */
static void append_float(char *out, int *pos, float value, int decimals)
{
    static const int32_t pow10[7] = {1, 10, 100, 1000, 10000, 100000, 1000000};
    int n;
    int32_t total;
    int32_t ip;
    int frac;
    int i;
    int32_t zpart;
    char tmp[24];
    int t = 0;

    if (value < 0.0f)
    {
        tmp[t++] = '-';
        value = -value;
    }

    total = (int32_t)(value * (float)pow10[decimals] + 0.5f);
    ip = total / pow10[decimals];
    frac = total % pow10[decimals];

    {
        char digits[16];
        int d = 0;
        if (ip == 0)
        {
            digits[d++] = '0';
        }
        else
        {
            while (ip > 0)
            {
                digits[d++] = (char)('0' + ip % 10);
                ip /= 10;
            }
        }
        for (i = d - 1; i >= 0; i--)
            tmp[t++] = digits[i];
    }

    if (decimals > 0)
    {
        tmp[t++] = '.';
        zpart = pow10[decimals];
        while (zpart > 1)
        {
            zpart /= 10;
            tmp[t++] = (char)('0' + (frac / zpart) % 10);
        }
    }
    tmp[t] = '\0';

    n = *pos + t < FRAME_CAPACITY - 1 ? t : FRAME_CAPACITY - 1 - *pos;
    for (i = 0; i < n; i++)
        out[(*pos)++] = tmp[i];
}

static void append_char(char *out, int *pos, char c)
{
    if (*pos < FRAME_CAPACITY - 1)
        out[(*pos)++] = (uint8_t)c;
}

static void append_int(char *out, int *pos, int32_t value)
{
    int n;
    char tmp[16];
    int i;
    if (value == 0)
    {
        append_char(out, pos, '0');
        return;
    }
    n = snprintf(tmp, sizeof(tmp), "%ld", (long)value);
    if (n > 0)
    {
        for (i = 0; i < n && *pos < FRAME_CAPACITY - 1; i++)
            out[(*pos)++] = (uint8_t)tmp[i];
    }
}

void app_protocol_send(const atk_ms901m_data_t *data)
{
    uint8_t frame[FRAME_CAPACITY];
    int n = 0;
    uint8_t cs;
    int i;
    const char hex[] = "0123456789ABCDEF";

    append_char((char *)frame, &n, '$');
    {
        const char *tag = "MS901";
        for (i = 0; tag[i] != '\0'; i++)
            append_char((char *)frame, &n, tag[i]);
    }

    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->attitude.roll, 2);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->attitude.pitch, 2);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->attitude.yaw, 2);

    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->gyro.x, 2);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->gyro.y, 2);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->gyro.z, 2);

    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->acc.x, 3);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->acc.y, 3);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->acc.z, 3);

    append_char((char *)frame, &n, ',');
    append_int((char *)frame, &n, data->mag.x);
    append_char((char *)frame, &n, ',');
    append_int((char *)frame, &n, data->mag.y);
    append_char((char *)frame, &n, ',');
    append_int((char *)frame, &n, data->mag.z);

    append_char((char *)frame, &n, ',');
    append_int((char *)frame, &n, data->baro.pressure);
    append_char((char *)frame, &n, ',');
    append_int((char *)frame, &n, data->baro.altitude);
    append_char((char *)frame, &n, ',');
    append_float((char *)frame, &n, data->baro.temperature, 2);

    /* 计算 '$' 与 '*' 之间（不含 '$'，即第 1 字节之后）的异或 */
    cs = 0;
    for (i = 1; i < n; i++)
        cs ^= frame[i];

    append_char((char *)frame, &n, '*');
    append_char((char *)frame, &n, hex[(cs >> 4) & 0x0F]);
    append_char((char *)frame, &n, hex[cs & 0x0F]);
    append_char((char *)frame, &n, '\r');
    append_char((char *)frame, &n, '\n');

    Serial_SendArray(frame, (uint16_t)n);
}
