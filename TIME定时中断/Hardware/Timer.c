#include "Timer.h"
extern uint16_t num;

void TimerInit()
{
	// 第一步 初始化通用时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
	// 默认配置内部时钟
	TIM_InternalClockConfig(TIM2);
	// 配置时基单元
	TIM_TimeBaseInitTypeDef TimeBaseInitTypeDef;
	TimeBaseInitTypeDef.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseInitTypeDef.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseInitTypeDef.TIM_Period = 10000 - 1;
	TimeBaseInitTypeDef.TIM_Prescaler = 7200 - 1;
	TimeBaseInitTypeDef.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM2,&TimeBaseInitTypeDef);
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	// 使能中断
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



void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update) == SET)
	{
		num++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}