#include "Timer.h"
uint16_t num;

void TimerInit()
{
	GPIO_InitTypeDef GPIO_InitStructure;
	// 第一步 初始化通用时钟
	//RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	// 初始化引脚
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

	//使用外部时钟
	TIM_ETRClockMode2Config(TIM2,TIM_ExtTRGPSC_OFF,TIM_ExtTRGPolarity_NonInverted,0x00);
	// 配置时基单元
	TIM_TimeBaseInitTypeDef TimeBaseInitTypeDef;
	TimeBaseInitTypeDef.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseInitTypeDef.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseInitTypeDef.TIM_Period = 10 - 1;
	TimeBaseInitTypeDef.TIM_Prescaler = 1 - 1;
	TimeBaseInitTypeDef.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2,&TimeBaseInitTypeDef);
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	// 中断输出控制
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);
	// NVIC
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVICTypeDef;
	NVICTypeDef.NVIC_IRQChannel = TIM2_IRQn;
	NVICTypeDef.NVIC_IRQChannelCmd = ENABLE;
	NVICTypeDef.NVIC_IRQChannelPreemptionPriority = 2;
	NVICTypeDef.NVIC_IRQChannelSubPriority = 1;
	NVIC_Init(&NVICTypeDef);
	
	TIM_Cmd(TIM2,ENABLE);
}


uint16_t Timer_GetCount()
{
	return TIM_GetCounter(TIM2);
}
// 中断函数
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update) == SET)
	{
		num++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}