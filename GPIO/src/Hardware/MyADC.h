#pragma once
#include "stm32f10x.h"

#define ADC_CLOCK_DIV RCC_PCLK2_Div6 // 6分频
#define ADC_CH      ADC_Channel_0
#define ADC_PIN     GPIO_Pin_0
#define ADC_PORT    GPIOA

#define TIM_PERIOD 999
#define ADC_BUF_LEN 1
extern uint16_t adc_buf[ADC_BUF_LEN];
extern uint16_t adc_snapshot[ADC_BUF_LEN];
extern volatile uint8_t adc_ready;
void RCC_CONFIG(void);
void GPIO_Config(void);
void DMA_Config(void);
void MyADC_Init();
void TIM3_Config(void);
void RCC_CONFIG();
void DMA_NVIC_Config();