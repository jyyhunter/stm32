#include "pwm.h"

void pwm_init()
{
	// 第一步 初始化通用时钟
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);		//开启AFIO的时钟，外部中断必须开启AFIO的时钟
	/*GPIO初始化*/
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;// 复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);			
	// 默认配置内部时钟
	TIM_InternalClockConfig(TIM2);
	// 配置时基单元，50Hz的频率
	TIM_TimeBaseInitTypeDef TimeBaseInitTypeDef;
	TimeBaseInitTypeDef.TIM_ClockDivision = TIM_CKD_DIV1;
	TimeBaseInitTypeDef.TIM_CounterMode = TIM_CounterMode_Up;
	TimeBaseInitTypeDef.TIM_Period = 20000 - 1; // ARR决定分辨率
	TimeBaseInitTypeDef.TIM_Prescaler = 72 - 1; //PSC；预分频器，影响频率
	TimeBaseInitTypeDef.TIM_RepetitionCounter = 0;   
	TIM_TimeBaseInit(TIM2,&TimeBaseInitTypeDef);
	
	// 初始化输出比较单元
	TIM_OCInitTypeDef TIM_OCIStucture;
	TIM_OCStructInit(&TIM_OCIStucture);
	TIM_OCIStucture.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCIStucture.TIM_OCPolarity = TIM_OCPolarity_High;
	TIM_OCIStucture.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCIStucture.TIM_Pulse = 0; //CCR的数值,决定占空比的大小
	//TIM_OC1Init(TIM2,&TIM_OCIStucture);
	// 输出比较定时器
	TIM_OC2Init(TIM2,&TIM_OCIStucture);
	TIM_Cmd(TIM2,ENABLE);
}

void PWM_SetCompare2(uint16_t Compare)
{
	// 修改占空比
	TIM_SetCompare2(TIM2,Compare);
}

void PWM_SetPreScaler(uint16_t upreScaler)
{
	TIM_PrescalerConfig(TIM2, upreScaler,TIM_PSCReloadMode_Immediate);
}