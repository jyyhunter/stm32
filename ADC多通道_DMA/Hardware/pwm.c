#include "pwm.h"
static uint32_t fac_us = 0;	//微秒倍乘数
static uint32_t fac_ms = 0;	//毫秒倍乘数



void pwm_init()
{
	// 1. 开启时钟：TIM4(APB1) + GPIOB(APB2)
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
	
	/*GPIO初始化 PB6 TIM4_CH1 复用推挽输出*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);			
	
	//定时器使用内部时钟
	TIM_InternalClockConfig(TIM4);
	
	// 时基单元配置：72M，PSC=719，ARR=99 → PWM频率1kHz，分辨率100级
	TIM_TimeBaseInitTypeDef TimeBaseInitTypeDef;
	TimeBaseInitTypeDef.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseInitTypeDef.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseInitTypeDef.TIM_Period = 100 - 1;    // ARR=99
	TimeBaseInitTypeDef.TIM_Prescaler = 720 - 1; // PSC=719
	TimeBaseInitTypeDef.TIM_RepetitionCounter = 0;
	TIM_TimeBaseInit(TIM4, &TimeBaseInitTypeDef);
	
	// 输出比较单元 TIM4_CH1
	TIM_OCInitTypeDef TIM_OCStructure;
	TIM_OCStructInit(&TIM_OCStructure);
	TIM_OCStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCStructure.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCStructure.TIM_Pulse = 20; //初始CCR=50，占空比50%
	TIM_OC1Init(TIM4, &TIM_OCStructure);
	
	//使能定时器
	TIM_Cmd(TIM4, ENABLE);
}

//修改占空比函数
void PWM_SetCompare1(uint16_t Compare)
{
	TIM_SetCompare1(TIM4, Compare);
}
//毫秒延时（你呼吸灯要用）
void delay_ms(uint32_t ms)
{
	uint32_t temp;
	SysTick->LOAD = ms * fac_ms;
	SysTick->VAL = 0x00;
	SysTick->CTRL |= SysTick_CTRL_ENABLE_Msk;
	do
	{
		temp = SysTick->CTRL;
	} while((temp & 0x01) && !(temp & (1 << 16)));
	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
	SysTick->VAL = 0x00;
}