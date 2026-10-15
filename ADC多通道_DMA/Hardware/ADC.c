#include "ADC.h"


uint16_t AD_Value[4];
void AD_Init()
{
	// 开启RCC时钟.初始化总线
	/* RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 6分频
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	
	// 配置GPIO
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;	//AIN 专属于ADC模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);		// 模拟输入引脚	
	// 配置选择,配置输入通道
	// 点菜菜单
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,2,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,3,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_3,4,ADC_SampleTime_55Cycles5);
	
	// 用结构体配置ADC转换器
	ADC_InitTypeDef ADC_InitDef;
	ADC_InitDef.ADC_Mode= ADC_Mode_Independent;
	ADC_InitDef.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitDef.ADC_ContinuousConvMode = ENABLE;
	ADC_InitDef.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitDef.ADC_NbrOfChannel = 4;
	ADC_InitDef.ADC_ScanConvMode = ENABLE; // ADC扫描模式
	ADC_Init(ADC1,&ADC_InitDef);
	
	// 配置中断，模拟看门狗，本次省略
	
	// 初始化DMA
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)ADC1->DR;        // 外设地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;             // 内存地址 ✔
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;        // 外设 → 内存
	
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
	
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable; // 外设地址不自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;         // 内存地址自增 ✔
	
	DMA_InitStructure.DMA_BufferSize = 4;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;        // 正常模式，传输完停止
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;          // 硬件触发
	
	DMA_Init(DMA1_Channel1,&DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1,ENABLE);
	// 开启ADC到DMA的使能
	ADC_DMACmd(ADC1,ENABLE);
	// 启用
	ADC_Cmd(ADC1,ENABLE);
	
	// 校准ADC
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1)== SET);
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));
	// 连续模式，触发一次即可
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	//ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	*/
	// 开启RCC时钟.初始化总线
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	RCC_ADCCLKConfig(RCC_PCLK2_Div6); // 72M/6=12M ADC时钟，F1最大ADC时钟14M，没问题
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);

	// 配置GPIO PA0~PA3 模拟量输入
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);		// ✅ 修改为GPIOA

	// 配置ADC规则通道
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_1,2,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_2,3,ADC_SampleTime_55Cycles5);
	ADC_RegularChannelConfig(ADC1,ADC_Channel_3,4,ADC_SampleTime_55Cycles5);

	// ADC结构体
	ADC_InitTypeDef ADC_InitDef;
	ADC_InitDef.ADC_Mode= ADC_Mode_Independent;
	ADC_InitDef.ADC_DataAlign = ADC_DataAlign_Right;
	ADC_InitDef.ADC_ContinuousConvMode = ENABLE;
	ADC_InitDef.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;
	ADC_InitDef.ADC_NbrOfChannel = 4;
	ADC_InitDef.ADC_ScanConvMode = ENABLE;
	ADC_Init(ADC1,&ADC_InitDef);

	// DMA初始化
	DMA_InitTypeDef DMA_InitStructure;
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)AD_Value;
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;

	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;

	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;

	DMA_InitStructure.DMA_BufferSize = 4;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

	DMA_Init(DMA1_Channel1,&DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1,ENABLE);

	ADC_Cmd(ADC1,ENABLE);
	// ADC校准
	ADC_ResetCalibration(ADC1);
	while(ADC_GetResetCalibrationStatus(ADC1));
	ADC_StartCalibration(ADC1);
	while(ADC_GetCalibrationStatus(ADC1));

	ADC_DMACmd(ADC1,ENABLE);
	ADC_SoftwareStartConvCmd(ADC1,ENABLE);

	
}
/*void AD_GetValue()
{
	DMA_Cmd(DMA1_Channel1,DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel1,4);
	DMA_Cmd(DMA1_Channel1,ENABLE);
	
	while(DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET);
	DMA_ClearFlag(DMA1_FLAG_TC1);
}*/
