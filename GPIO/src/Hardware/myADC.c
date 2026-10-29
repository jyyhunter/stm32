#include "MyADC.h"
#include "string.h"
uint16_t adc_buf[ADC_BUF_LEN];
uint16_t adc_snapshot[ADC_BUF_LEN];
volatile uint8_t adc_ready = 0;

// 配置时钟
void RCC_CONFIG()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3,ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
}

// 配置GPIO
void GPIO_Config()
{
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStruct.GPIO_Pin = ADC_PIN;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ADC_PORT,&GPIO_InitStruct);
}
// 配置DMA
void DMA_Config()
{
    DMA_InitTypeDef DMA_InitStruct;
    DMA_DeInit(DMA1_Channel1);

    DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;// 外设源地址：ADC数据寄存器DR
    DMA_InitStruct.DMA_MemoryBaseAddr = (uint32_t)adc_buf;      // 内存目标地址：采样缓存数组
    DMA_InitStruct.DMA_DIR = DMA_DIR_PeripheralSRC;             // 传输方向：外设 → 内存
    DMA_InitStruct.DMA_BufferSize = ADC_BUF_LEN;                // 一次传输数据长度（单通道=1）
    DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址固定，不自增（一直读DR）
    DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;        // 内存地址固定，不自增（一直写adc_buf[0]）
    DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord; // 外设数据宽度：半字16bit
    DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;         // 内存数据宽度：半字16bit
    DMA_InitStruct.DMA_Mode = DMA_Mode_Circular;                 // 循环模式：持续往复传输
    DMA_InitStruct.DMA_Priority = DMA_Priority_High;             // DMA优先级：高，保证采集实时性
    DMA_InitStruct.DMA_M2M = DMA_M2M_Disable;                    // 关闭内存到内存模式，开启外设请求模式
    DMA_Init(DMA1_Channel1, &DMA_InitStruct);
    
    DMA_ITConfig(DMA1_Channel1,DMA_IT_TC,ENABLE); // 开启DMA传输完成中断
    DMA_Cmd(DMA1_Channel1,ENABLE);

}
// 开启NVIC中断配置
void DMA_NVIC_Config()
{
    NVIC_InitTypeDef NVIC_InitStruct;
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
    NVIC_InitStruct.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
    
    NVIC_Init(&NVIC_InitStruct);
}
//
void MyADC_Init()
{
    ADC_InitTypeDef ADC_InitStruct;
    ADC_InitStruct.ADC_ContinuousConvMode = DISABLE;//外部触发不需要连续转换
    ADC_InitStruct.ADC_Mode = ADC_Mode_Independent; //ADC独立模式，不使用ADC1+ADC2双ADC同步
    ADC_InitStruct.ADC_ScanConvMode = DISABLE; // 单通道关闭扫描
    ADC_InitStruct.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;//触发源选择：TIM3的TRGO信号
    ADC_InitStruct.ADC_DataAlign = ADC_DataAlign_Right;// 右对齐
    ADC_InitStruct.ADC_NbrOfChannel = 1; //规则通道数量 1
    ADC_Init(ADC1,&ADC_InitStruct);
    // 配置ADC相关采样周期，通道
    ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_239Cycles5);
    // 使能ADC和DMA建立联系
    ADC_DMACmd(ADC1,ENABLE);
    // 开启外部中断
    ADC_ExternalTrigConvCmd(ADC1,ENABLE);
    ADC_Cmd(ADC1,ENABLE);

    //ADC校准
    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1));
    ADC_StartCalibration(ADC1);
    while(ADC_GetCalibrationStatus(ADC1));
    
}

void TIM3_Config()
{
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_DeInit(TIM3);

    TIM_InitStruct.TIM_Prescaler = 71; //  预分频
    TIM_InitStruct.TIM_Period = TIM_PERIOD; // 自动重装
    TIM_InitStruct.TIM_ClockDivision = TIM_CKD_DIV1; // 
    TIM_InitStruct.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM3,&TIM_InitStruct);

    TIM_SelectOutputTrigger(TIM3,TIM_TRGOSource_Update);
    TIM_Cmd(TIM3,ENABLE);
}


void DMA1_Channel1_IRQHandler(void)
{
    if(DMA_GetITStatus(DMA1_IT_TC1) != RESET)
    {
        DMA_ClearITPendingBit(DMA1_IT_TC1);
        memcpy(adc_snapshot,adc_buf,sizeof(adc_buf));
        adc_ready = 1;
    }
}