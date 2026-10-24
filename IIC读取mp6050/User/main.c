#include "stm32f10x.h"
#include <stdio.h>
#include "Delay.h"
#include "Serial.h"
#include "Tick.h"
#include "AS5600.h"
#include "OLED.h"
#include "Speed.h"
#include "MPU6050.h"

int main(void)
{
    IIC_Init();
    IIC_Start();
    // 扫描总线上的所有设备
    // IIC_SendByte(0x6C);// 寻址
    // uint8_t ack = IIC_ReceiveAck();
    // IIC_Stop();
    // OLED_Init();
    // OLED_ShowNum(1,1,ack,3);
    MPU_Init();

    uint8_t ack =MPU_ReadReg(0x6D);
    OLED_ShowNum(1,1,ack,3);
    while(1)
    {
        
    }
    
}
