#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms
#include "coder.h"
#include "OLED.h"
int main(void)
{
	OLED_Init();
	coder_init();
	OLED_ShowString(1,1,"CNT:");
	while(1)
	{
		OLED_ShowSignedNum(1,5,coder_get(),5);
		Delay_ms(500);
	}
}
