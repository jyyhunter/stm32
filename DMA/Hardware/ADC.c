#include "ADC.h"

void AD_Init()
{
	// 开启RCC时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 6分频
	
	// 配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	//AIN 专属于ADC模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB, &GPIO_InitStructure);		// 模拟输入引脚	
	// 配置选择,配置输入通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);
	
	// 用结构体配置ADC转换器
	ADC_InitTypeDef ADC_InitDef;
	ADC_InitDef.ADC_Mode= ADC_Mode_Independent; // 独立模式
	ADC_InitDef.ADC_DataAlign = ADC_DataAlign_Right; // 数据右对齐
	ADC_InitDef.ADC_ContinuousConvMode = DISABLE; // 单次转换模式s
	ADC_InitDef.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None; // 软件触发
	ADC_InitDef.ADC_NbrOfChannel = 1; // 通道数量
	ADC_InitDef.ADC_ScanConvMode = DISABLE; // 扫描模式关闭
	ADC_Init(ADC1,&ADC_InitDef);
	
	// 配置中断，模拟看门狗，本次省略
	
	
	
	// 启用
	ADC_Cmd(ADC1,ENABLE); // 使能ADC1
	// 校准ADC
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1)== SET);
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
}
uint16_t AD_GetValue()
{
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	while(ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)==RESET); // 5.6 us等待
	return ADC_GetConversionValue(ADC1);
}
