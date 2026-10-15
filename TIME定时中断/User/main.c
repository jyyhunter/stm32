#include "stm32f10x.h"                  // Device header
#include "OLED.h"
#include "TIMER.h"


uint16_t num;
int main(void)
{
	OLED_Init();
	TimerInit();
	OLED_ShowString(1,1,"num:");
	
	
	while (1)
	{
		OLED_ShowNum(1,5,num,3);
	}
	
}
