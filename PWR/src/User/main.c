#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "stm32f10x_usart.h"



// 独立看门狗服务、窗口看门狗能够实现硬件故障的条件下的设备复位等；

int main(void)
{
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR,ENABLE);
    // 1、开启独立看门狗的时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_WWDG,ENABLE);
    WWDG_SetPrescaler(WWDG_Prescaler_8); // 8分频
    WWDG_SetWindowValue(0x40 | 21); // 30 ms
    WWDG_Enable(0x40 | 54);   // 50ms
    while (1)
    {
       
        //__WFI(); // cpu立刻睡眠，可以通过中断唤醒设备
        PWR_EnterSTOPMode(PWR_Regulator_ON,PWR_STOPEntry_WFI);


         // 喂狗，窗口喂狗的时间不能太短
        WWDG_SetCounter(0x40 | 54);
        SystemInit();// 睡眠会导致时钟变为HSI 8Mhz，通过调用这个指令，将频率重新调回72Mhz
    }
}
