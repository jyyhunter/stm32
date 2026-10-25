#include "MyRTC.h"
#include <time.h>


uint16_t MyTime[] = {2026,1,1,29,59,59};
void MyRTC_Init()
{
    // 第一步
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR | RCC_APB1Periph_BKP, ENABLE); // 使能PWR和BKP时钟
    PWR_BackupAccessCmd(ENABLE); // 允许访问备份寄存器

    // 是否有备用电池的检测
    if (BKP_ReadBackupRegister(BKP_DR1) == RESET)
    {
        /* code */
        RCC_LSEConfig(RCC_LSE_ON);
        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET);
        
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE);
        RCC_RTCCLKCmd(ENABLE);

        RTC_WaitForSynchro();
        RTC_WaitForLastTask();

        RTC_SetPrescaler(32768 - 1);
        RTC_WaitForLastTask();

        MyRTC_SetTime();

        BKP_WriteBackupRegister(BKP_DR1,0xA5A5);
    }else
    {
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
    }
    
    // 第二步
    RCC_LSEConfig(RCC_LSE_ON); // 开启外部低速晶振
    while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // 等待LSE稳定
    // 第三步
    RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); // 选择LSE作为RTC时钟源
    // 第四步
    RCC_RTCCLKCmd(ENABLE); // 使能RTC时钟

    RTC_WaitForSynchro(); // 等待RTC寄存器同步
    RTC_WaitForLastTask(); // 等待上一次操作完成
    // 设置分频器
    RTC_SetPrescaler(32768 - 1); // 设置RTC预分频器，1Hz
    RTC_WaitForLastTask(); // 等待上一次操作完成
    // 
    RTC_SetCounter(1672588795); // 设置RTC计数器初值
    RTC_WaitForLastTask(); // 等待上一次操作完成
    //

}

void MyRTC_SetTime()
{
    time_t time_Cnt;
    struct tm time_date;
    time_date.tm_year = MyTime[0] - 1900;
    time_date.tm_mon = MyTime[1] - 1;
    time_date.tm_mday = MyTime[2];
    time_date.tm_hour = MyTime[3];
    time_date.tm_min = MyTime[4];
    time_date.tm_sec = MyTime[5];
    
    time_Cnt = mktime(&time_date) - 8 * 60 * 60;
    RTC_SetCounter(time_Cnt);
    RTC_WaitForLastTask();
}

void MyRTC_ReadTime()
{
    time_t time_cnt;
    struct tm time_date;
    time_cnt = RTC_GetCounter() + 8 * 60 * 60;
    time_date= *localtime(&time_cnt);
    MyTime[0] = time_date.tm_year + 1900;
    MyTime[1] = time_date.tm_mon + 1;
    MyTime[2] = time_date.tm_mday;
    MyTime[3] = time_date.tm_hour;
    MyTime[4] = time_date.tm_min;
    MyTime[5] = time_date.tm_sec;   
}