#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "Serial.h"
#include "Tick.h"
#include "AS5600.h"
#include "OLED.h"
#include "Speed.h"

/* 任务调度周期 */
#define SAMPLE_PERIOD_MS   50U    /* 速度采样周期（与 Speed 模块一致） */
#define REPORT_PERIOD_MS   100U   /* OLED 刷新与串口上报周期 */

/* AS5600 状态寄存器位 */
#define AS5600_MD_BIT      0x20   /* bit5: 磁铁在位 */
#define AS5600_ML_BIT      0x10   /* bit4: 磁场过弱 */
#define AS5600_MH_BIT      0x08   /* bit3: 磁场过强 */

/* 取绝对值（避免对 INT32_MIN 用 abs 未定义） */
static uint32_t Abs32(int32_t x)
{
    return (x >= 0) ? (uint32_t)x : (uint32_t)(-x);
}

int main(void)
{
    uint32_t t_last_sample;
    uint32_t t_last_report;
    uint8_t  magnet_ok;
    uint8_t  oled_addr;

    Serial_Init(115200);
    Delay_ms(500);
    OLED_Init();              /* 内部自动扫描 0x78/0x7A，并完成推挽式软件 I2C 初始化 */
    oled_addr = OLED_GetAddr();

    Speed_Init();
    Tick_Init();

    /* OLED 固定标签（仅开机画一次） */
    OLED_ShowString(1, 1, "AS5600  SPEED");
    OLED_ShowString(2, 1, "RPM");
    OLED_ShowString(3, 1, "ANG");
    OLED_ShowString(4, 1, "D/s");
    OLED_ShowChar(2, 9, '.');
    OLED_ShowChar(3, 8, '.');
    OLED_ShowChar(4, 10, '.');

    magnet_ok = (Speed_GetStatus() & AS5600_MD_BIT) ? 1 : 0;

    printf("\r\n");
    printf("=====================================\r\n");
    printf("  AS5600 Speed Sensor (STM32F103VET6)\r\n");
    printf("  AS5600 I2C: SCL=PB6 SDA=PB7 (0x36)\r\n");
    if (OLED_IsReady())
    {
        printf("  OLED  I2C: SCL=PB8 SDA=PB9, found at 0x%02X\r\n", oled_addr);
    }
    else
    {
        printf("  OLED  I2C: SCL=PB8 SDA=PB9, NOT FOUND! Check SCL/SDA,VCC,address\r\n");
    }
    printf("  UART: USART3 PB10(TX) PB11(RX),115200\r\n");
    printf("  Sample=%dms, Window=%dms, Range ~ +/-600rpm\r\n",
           SPEED_SAMPLE_MS, SPEED_SAMPLE_MS * SPEED_WINDOW_N);
    printf("  Magnet at power-on: %s\r\n", magnet_ok ? "YES" : "NO");
    printf("=====================================\r\n");

    t_last_sample = g_tick_ms;
    t_last_report = g_tick_ms;

    while (1)
    {
        /* 固定周期采样 */
        if (g_tick_ms - t_last_sample >= SAMPLE_PERIOD_MS)
        {
            t_last_sample += SAMPLE_PERIOD_MS;
            Speed_Sample();
        }

        /* 固定周期刷新 OLED + 串口上报 */
        if (g_tick_ms - t_last_report >= REPORT_PERIOD_MS)
        {
            int32_t rc = Speed_GetRpmCenti();    /* 转速 x100 */
            int32_t ac = Speed_GetAngleCenti();  /* 角度 x100 */
            int32_t dc = Speed_GetDpsTenth();    /* 角速度 x10 */
            uint8_t status = Speed_GetStatus();
            uint32_t ra, aa, da;

            ra = Abs32(rc);
            aa = Abs32(ac);
            da = Abs32(dc);

            /* ---------- OLED ---------- */
            if (OLED_IsReady())
            {
            /* 第1行：磁铁状态 */
            if (status & AS5600_MD_BIT)
            {
                OLED_ShowString(1, 14, "OK ");
            }
            else
            {
                OLED_ShowString(1, 14, "NO ");
            }
            if (status & AS5600_MH_BIT)
            {
                OLED_ShowString(1, 14, "HI ");   /* 磁场过强 */
            }
            else if (status & AS5600_ML_BIT)
            {
                OLED_ShowString(1, 14, "LO ");   /* 磁场过弱 */
            }

            /* 第2行：RPM，符号+3位整数+小数点+2位小数（小数点固定显示） */
            OLED_ShowSignedNum(2, 5, rc / 100, 3);
            OLED_ShowNum(2, 10, ra % 100, 2);

            /* 第3行：ANG，3位整数+小数点+2位小数 */
            OLED_ShowNum(3, 5, (uint32_t)(aa / 100), 3);
            OLED_ShowNum(3, 9, aa % 100, 2);

            /* 第4行：D/s，符号+4位整数+小数点+1位小数 */
            OLED_ShowSignedNum(4, 5, dc / 10, 4);
            OLED_ShowNum(4, 11, da % 10, 1);
            }

            /* ---------- 串口 ---------- */
            printf("RPM=%c%03lu.%02lu  ANG=%03lu.%02lu  DPS=%c%04lu.%01lu  REV=%ld\r\n",
                   (rc >= 0) ? '+' : '-', ra / 100, ra % 100,
                   aa / 100, aa % 100,
                   (dc >= 0) ? '+' : '-', da / 10, da % 10,
                   (long)Speed_GetRevolutions());

            t_last_report += REPORT_PERIOD_MS;
        }
    }
}
