#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms
#include "OLED.h"
#include "ADC.h"

uint16_t ADValue;
float Voltage;
int main(void)
{
	AD_Init();
	OLED_Init();
	OLED_ShowString(1,1,"ADValue:");
	OLED_ShowString(2,1,"Volatge:0.00V");
	while(1)
	{
		ADValue = AD_GetValue();
		Voltage = (float)ADValue / 4095 * 3.3;
		OLED_ShowSignedNum(1,9,ADValue,5);
		OLED_ShowNum(2,9,Voltage,5);
	}
}
