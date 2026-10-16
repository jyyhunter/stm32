#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms
#include "OLED.h"
#include "ADC.h"
#include "serial.h"
#include "Key.h"

uint8_t keyNum;
int main(void)
{
	OLED_Init();
	Serial_Init();
	OLED_ShowString(1,1,"TxPacket");
	OLED_ShowString(3,1,"RxPacket");
	
	Serial_TxPacket[0] = 0x01;
	Serial_TxPacket[1] = 0x02;
	Serial_TxPacket[2] = 0x03;
	Serial_TxPacket[3] = 0x04;
	while(1)
	{
		keyNum = Key_GetNum();
		if(keyNum==1)
		{
			Serial_TxPacket[0]++;
			Serial_TxPacket[1]++;
			Serial_TxPacket[2]++;
			Serial_TxPacket[3]++;
			Serial_SendPacket();
			// OLED显示代码
		}else{
		
			//显示代码
		
		}
	}
}
