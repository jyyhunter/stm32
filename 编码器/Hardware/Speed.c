#include "Speed.h"
#include "AS5600.h"

static int32_t  s_last_raw;   /* 上一次原始角度 0~4095 */
static int32_t  s_total;      /* 累计位置（带符号刻度） */
static int32_t  s_buf[SPEED_WINDOW_N];  /* 滑动窗口增量缓冲 */
static uint8_t  s_idx;        /* 窗口写指针 */
static int32_t  s_sum;        /* 窗口内增量之和 */
static uint16_t s_raw;        /* 最新原始角度 */

void Speed_Init(void)
{
    uint8_t i;

    AS5600_Init();

    s_last_raw = (int32_t)AS5600_GetRawAngle();
    s_raw      = (uint16_t)s_last_raw;
    s_total    = 0;
    s_sum      = 0;
    s_idx      = 0;
    for (i = 0; i < SPEED_WINDOW_N; i++)
    {
        s_buf[i] = 0;
    }
}

/* 周期性采样：计算带符号增量并维护滑动窗口 */
void Speed_Sample(void)
{
    int32_t delta;

    s_raw = AS5600_GetRawAngle();

    delta = (int32_t)s_raw - s_last_raw;
    if (delta > 2047)
    {
        delta -= COUNTS_PER_REV;      /* 正向跨过 0 点 */
    }
    else if (delta < -2047)
    {
        delta += COUNTS_PER_REV;      /* 反向跨过 0 点 */
    }

    s_last_raw = s_raw;
    s_total += delta;

    /* 滑动窗口：先减去最旧的，再写入最新的 */
    s_sum -= s_buf[s_idx];
    s_buf[s_idx] = delta;
    s_sum += delta;
    s_idx = (uint8_t)((s_idx + 1U) % SPEED_WINDOW_N);
}

/*
 * RPM = 窗口增量 / 每圈刻度 / 窗口时长(秒) × 60
 *     窗口时长 = 4 × 50ms = 0.2s
 *     RPM      = s_sum / 4096 / 0.2 × 60 = s_sum × 300 / 4096
 *     RPM×100  = s_sum × 30000 / 4096
 */
int32_t Speed_GetRpmCenti(void)
{
    return s_sum * 30000 / COUNTS_PER_REV;
}

/*
 * 角速度(度/秒) = s_sum / 4096 × 360 / 0.2 = s_sum × 1800 / 4096
 * 角速度×10     = s_sum × 18000 / 4096
 */
int32_t Speed_GetDpsTenth(void)
{
    return s_sum * 18000 / COUNTS_PER_REV;
}

/* 角度(度)×100 = raw × 360 × 100 / 4096 = raw × 36000 / 4096 */
int32_t Speed_GetAngleCenti(void)
{
    return (int32_t)s_raw * 36000 / COUNTS_PER_REV;
}

int32_t Speed_GetTotalCount(void)
{
    return s_total;
}

int32_t Speed_GetRevolutions(void)
{
    return s_total / COUNTS_PER_REV;
}

uint8_t Speed_GetStatus(void)
{
    return AS5600_GetStatus();
}
