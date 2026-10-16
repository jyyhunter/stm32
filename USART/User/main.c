#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms
#include "OLED.h"
#include "ADC.h"
#include "serial.h"


uint8_t RxData;
int main(void)
{
	OLED_Init();
	Serial_Init();
	Serial_SendByte('A');
	while(1)
	{
		if(USART_GetFlagStatus(USART1,USART_FLAG_RXNE) == SET)
		{
			RxData = USART_ReceiveData(USART1);

		}
	}
}
