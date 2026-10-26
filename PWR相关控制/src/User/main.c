#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "stm32f10x_usart.h"
#include "time.h"
#include  "OLED.h"
#include "stm32f10x_bkp.h"
#include "OLED.h"
/*
 * 硬件连接：
 *   ATK-MS901M 模块：TX->PB11(USART3_RX)  RX->PB10(USART3_TX)  VCC/GND
 *   USB-TTL 上位机  ：RXD->PA9(USART1_TX)  TXD->PA10(USART1_RX) GND
 *   波特率均为 115200，8N1。
 */
int main(void)
{
    // BKP初始化
    Serial_Init(); // 初始化串口1（上位机）和串口3（MS901）
    OLED_Init();

  
    while (1)
    {
       
        
       __WFI(); // 让芯片睡眠
    }

}
