#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "stm32f10x_usart.h"
#include "MYSPI.h"
#include "GPIO_TEST.h"
#include "time.h"
#include "stm32f10x_bkp.h"
#include "MyRTC.h"
/*
 * 硬件连接：
 *   ATK-MS901M 模块：TX->PB11(USART3_RX)  RX->PB10(USART3_TX)  VCC/GND
 *   USB-TTL 上位机  ：RXD->PA9(USART1_TX)  TXD->PA10(USART1_RX) GND
 *   波特率均为 115200，8N1。
 */
int main(void)
{
    // time_t start_time, current_time;
    // struct tm *timeinfo;
    // timeinfo.tm_year = 2024 - 1900; // 年份从1900开始计数
    // timeinfo.tm_mon = 5 - 1;        // 月份从0开始计数
    // timeinfo.tm_mday = 1;           // 日期
    // timeinfo.tm_hour = 0;           // 小时
    // timeinfo.tm_min = 0;            // 分钟
    // timeinfo.tm_sec = 0;            // 秒         
    // 开启pwr和BKP的时钟                                             
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE); // 使能备份寄存器时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE); // 使能备份寄存器时钟
    PWR_BackupAccessCmd(ENABLE); // 允许访问备份寄存器
    // 允许访问备份寄存器
    BKP_WriteBackupRegister(BKP_DR1, 0x1234); // 写入数据到备份寄存器
    // 断电后，通过备用电源（如电池）保持备份寄存器的数据
    BKP_ReadBackupRegister(BKP_DR1); // 读取备份寄存器的数据

    //time_t time_cnt = time(NULL); // 获取当前 时间戳
    //printf("Current time: %ld\r\n", time_cnt); // 打印当前时间
    // BKP初始化
    Serial_Init(); // 初始化串口1（上位机）和串口3（MS901）
    Serial_Printf("MS901M Test\r\n");
    //GPIO_InitT(); // 初始化 GPIOB 引脚
    MyRTC_Init(); // 初始化RTC, 开始计数
    while (1)
    {
        Serial_Printf("Current RTC Counter: %ld\r\n", RTC_GetCounter());
        
        // GPIO_WriteBit(GPIOB, GPIO_Pin_7, 1); // 设置 PB7 高电平
        // Delay_ms(1000);
        // GPIO_WriteBit(GPIOB, GPIO_Pin_7, 0); // 设置 PB7 低电平
        Serial_SendString("Hello, MS901M!\r\n");
        Delay_ms(1000);
    }

}
