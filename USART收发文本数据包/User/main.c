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
	
	
	while(1)
	{
		if(GetRx_Flag() == 1)
		{
			OLED_ShowString(4,3,Serial_RxPacket);
			
		}
		
	}
}
