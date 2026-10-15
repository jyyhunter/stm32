#include "BLDCDriver.h"

#define _CONSTRAIN(amt, low, high)  ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)))

/* 内部：设置某一相的占空比，dc 范围 0.0~1.0 */
static void Driver_WriteDuty(uint8_t phase, float dc)
{
    uint32_t ccr;
    dc = _CONSTRAIN(dc, 0.0f, 1.0f);
    ccr = (uint32_t)(dc * DRIVER_PWM_PERIOD);
    if (ccr > DRIVER_PWM_PERIOD) ccr = DRIVER_PWM_PERIOD;
    switch (phase)
    {
        case 0: TIM_SetCompare1(TIM1, ccr); break;
        case 1: TIM_SetCompare2(TIM1, ccr); break;
        case 2: TIM_SetCompare3(TIM1, ccr); break;
    }
}

void Driver_Init(BLDCDriver *driver, float Vpower)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    TIM_OCInitTypeDef TIM_OCInitStructure;

    driver->voltage_power_supply = Vpower;
    driver->voltage_limit = Vpower;

    /* 开启时钟 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOB |
                           RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);

    /* PA8/PA9/PA10 复用推挽输出（TIM1_CH1/2/3） */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8 | GPIO_Pin_9 | GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* PB13/PB14/PB15 推挽输出（三相使能） */
    GPIO_InitStructure.GPIO_Pin = DRIVER_EN_ALL_PINS;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DRIVER_EN_PORT, &GPIO_InitStructure);

    /* 初始使能脚拉低 */
    GPIO_ResetBits(DRIVER_EN_PORT, DRIVER_EN_ALL_PINS);

    /* TIM1 时基：向上计数，72MHz，20kHz */
    TIM_TimeBaseStructure.TIM_Period = DRIVER_PWM_PERIOD - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);

    /* 三路 PWM 模式1，初始 50% 占空比 */
    TIM_OCStructInit(&TIM_OCInitStructure);
    TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCInitStructure.TIM_Pulse = DRIVER_PWM_PERIOD / 2;
    TIM_OC1Init(TIM1, &TIM_OCInitStructure);
    TIM_OC2Init(TIM1, &TIM_OCInitStructure);
    TIM_OC3Init(TIM1, &TIM_OCInitStructure);

    TIM_OC1PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM1, TIM_OCPreload_Enable);
    TIM_ARRPreloadConfig(TIM1, ENABLE);

    /* 高级定时器必须开启主输出(MOE) */
    TIM_CtrlPWMOutputs(TIM1, ENABLE);
    TIM_Cmd(TIM1, ENABLE);
}

void Driver_Enable(BLDCDriver *driver)
{
    GPIO_SetBits(DRIVER_EN_PORT, DRIVER_EN_ALL_PINS);
    Driver_SetPwm(driver, 0.0f, 0.0f, 0.0f);
}

void Driver_Disable(BLDCDriver *driver)
{
    Driver_SetPwm(driver, 0.0f, 0.0f, 0.0f);
    GPIO_ResetBits(DRIVER_EN_PORT, DRIVER_EN_ALL_PINS);
}

void Driver_SetPwm(BLDCDriver *driver, float Ua, float Ub, float Uc)
{
    /* 限制到 0 ~ 电源电压 */
    Ua = _CONSTRAIN(Ua, 0.0f, driver->voltage_limit);
    Ub = _CONSTRAIN(Ub, 0.0f, driver->voltage_limit);
    Uc = _CONSTRAIN(Uc, 0.0f, driver->voltage_limit);

    /* 换算为占空比 0~1 */
    Driver_WriteDuty(0, Ua / driver->voltage_power_supply);
    Driver_WriteDuty(1, Ub / driver->voltage_power_supply);
    Driver_WriteDuty(2, Uc / driver->voltage_power_supply);
}
