#include "stm32f10x.h"
#include "pwm.h"
#include "delay.h"	//需要你工程有delay_ms
#include "OLED.h"
#include "MYDMA.h"

uint8_t dataA[] = {0x01,0x02,0x03,0x04};
uint8_t dataB[] = {0,0,0,0};

int main(void)
{
	
	OLED_Init();
	OLED_ShowHexNum(1,1,dataA[0],2);
	OLED_ShowHexNum(1,4,dataA[1],2);
	OLED_ShowHexNum(1,7,dataA[2],2);
	OLED_ShowHexNum(1,10,dataA[3],2);
	OLED_ShowHexNum(2,1,dataB[0],2);
	OLED_ShowHexNum(2,4,dataB[1],2);
	OLED_ShowHexNum(2,7,dataB[2],2);
	OLED_ShowHexNum(2,10,dataB[3],2);
	MYDMA_Init((uint32_t)dataA,(uint32_t)dataB,4);
	
	OLED_ShowHexNum(3,1,dataA[0],2);
	OLED_ShowHexNum(3,4,dataA[1],2);
	OLED_ShowHexNum(3,7,dataA[2],2);
	OLED_ShowHexNum(3,10,dataA[3],2);
	OLED_ShowHexNum(4,1,dataB[0],2);
	OLED_ShowHexNum(4,4,dataB[1],2);
	OLED_ShowHexNum(4,7,dataB[2],2);
	OLED_ShowHexNum(4,10,dataB[3],2);
	while(1)
	{
		
	}
}
