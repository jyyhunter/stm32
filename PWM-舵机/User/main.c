#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "TIMER.h"
#include "pwm.h"

uint8_t i;
int main(void)
{
	OLED_Init();
	pwm_init();
	PWM_SetCompare2(500);
	OLED_ShowString(1,1,"num:");
	while (1)
	{
		for(i = 0; i <=100;i++)
		{
			//PWM_SetCompare1(i);
			OLED_ShowNum(1,7,(uint32_t)i,4);
		}
	}
	
}
