#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms

int main(void)
{
	uint16_t duty = 0;
	int8_t dir = 1;
	pwm_init();
	
	while(1)
	{
		PWM_SetCompare1(duty);
		delay_ms(20);
		duty += dir;
		if(duty >= 99) dir = -1;
		if(duty <= 0) dir = 1;
	}
}
