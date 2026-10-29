#include "pwm.h"


void PWM_Init()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInstructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;
    // RCC_APB2Periph_AFIO用于终端引脚选择以及引脚复用功能
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO,ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB2Periph_TIM10,ENABLE);


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA,&GPIO_InitStructure);

    // 定时器时基 72MHz->72khz->1kHz
    TIM_TimeBaseInstructure.TIM_Period = 999; // ARR寄存器
    TIM_TimeBaseInstructure.TIM_Prescaler = 71; // psc
    TIM_TimeBaseInstructure.TIM_ClockDivision = TIM_CKD_DIV1; // 1分频
    TIM_TimeBaseInstructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInstructure);

    // pwm 输出配置
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_Pulse = 500;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OC1Init(TIM2,&TIM_OCInitStructure);

    // 预装载使能
    TIM_OC1PreloadConfig(TIM2,TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM2,ENABLE);
    // 开启时钟
    TIM_Cmd(TIM2,ENABLE);
}

void Set_Duty(uint16_t duty){
    if (duty > 100)
    {
        duty = 100;
    }
    TIM_SetCompare1(TIM2,(uint16_t)(999 * duty)/100);
}

// GPIO 引脚表：按 TIM2~TIM4 常用复用引脚，可自行扩充
static uint16_t pin_of(TIM_TypeDef *t, uint8_t ch) {
    if (t == TIM2) return GPIO_Pin_0 << (ch - 1);           // PA0~PA3
    if (t == TIM3) return (ch <= 2) ? (GPIO_Pin_6 << (ch - 1))     // PA6~PA7
                                    : (GPIO_Pin_0 << (ch - 3));    // PB0~PB1
    if (t == TIM4) return GPIO_Pin_6 << (ch - 1);           // PB6~PB9
    return 0;
}
static GPIO_TypeDef *gpio_of(TIM_TypeDef *t, uint8_t ch) {
    if (t == TIM3 && ch >= 3) return GPIOB;
    if (t == TIM4)            return GPIOB;
    return GPIOA;
}

// 初始化：开时钟 -> 配引脚 -> 设时基 -> 配 PWM -> 启动
void PWMCommon_Init(PWM_Ch *c)
{
    GPIO_InitTypeDef g;
    TIM_TimeBaseInitTypeDef tb;
    TIM_OCInitTypeDef oc;
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2 | RCC_APB1Periph_TIM3 | RCC_APB1Periph_TIM4, ENABLE);          // 全开省事
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);

    g.GPIO_Pin = pin_of(c->TIMx, c->ch);
    g.GPIO_Mode = GPIO_Mode_AF_PP;
    g.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(gpio_of(c->TIMx, c->ch), &g);

    tb.TIM_Period = c->arr;
    tb.TIM_Prescaler = c->psc;
    tb.TIM_ClockDivision = TIM_CKD_DIV1;
    tb.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(c->TIMx, &tb);

    oc.TIM_OCMode = TIM_OCMode_PWM1;
    oc.TIM_OutputState = TIM_OutputState_Enable;
    oc.TIM_Pulse = c->ccr;
    oc.TIM_OCPolarity = TIM_OCPolarity_High;
    switch (c->ch) {                     // 通道分发
        case 1: TIM_OC1Init(c->TIMx, &oc); TIM_OC1PreloadConfig(c->TIMx, TIM_OCPreload_Enable); break;
        case 2: TIM_OC2Init(c->TIMx, &oc); TIM_OC2PreloadConfig(c->TIMx, TIM_OCPreload_Enable); break;
        case 3: TIM_OC3Init(c->TIMx, &oc); TIM_OC3PreloadConfig(c->TIMx, TIM_OCPreload_Enable); break;
        case 4: TIM_OC4Init(c->TIMx, &oc); TIM_OC4PreloadConfig(c->TIMx, TIM_OCPreload_Enable); break;
    }
    TIM_ARRPreloadConfig(c->TIMx, ENABLE);
    TIM_Cmd(c->TIMx, ENABLE);
}

// 按百分比设占空比（0~100）
void PWM_SetDuty(PWM_Ch *c, uint16_t duty) {
    if (duty > 100) duty = 100;
    uint16_t ccr = (uint16_t)((uint32_t)c->arr * duty / 100);
    switch (c->ch) {
        case 1: TIM_SetCompare1(c->TIMx, ccr); break;
        case 2: TIM_SetCompare2(c->TIMx, ccr); break;
        case 3: TIM_SetCompare3(c->TIMx, ccr); break;
        case 4: TIM_SetCompare4(c->TIMx, ccr); break;
    }
}
