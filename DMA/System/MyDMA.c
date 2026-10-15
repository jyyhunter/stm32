#include "MyDMA.h"

uint16_t MYDMA_Size;
// DMA转运数据
void MYDMA_Init(uint32_t AdddrA,uint32_t ADDRB,uint16_t Size)
{
	MYDMA_Size = Size;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1,ENABLE);
	
	DMA_InitTypeDef DMA_InitStructure;
	
	DMA_InitStructure.DMA_PeripheralBaseAddr = AdddrA;        // 外设地址
	DMA_InitStructure.DMA_MemoryBaseAddr = ADDRB;             // 内存地址 ✔
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;        // 外设 → 内存
	
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Enable; // 外设地址自增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;         // 内存地址自增 ✔
	
	DMA_InitStructure.DMA_BufferSize = Size;
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;        // 正常模式，传输完停止
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStructure.DMA_M2M = DMA_M2M_Enable;          // 内存到内存模式（软件触发DMA）
	
	DMA_Init(DMA1_Channel1,&DMA_InitStructure);
	DMA_Cmd(DMA1_Channel1,ENABLE);
}



void MyDMA_Transfer()
{
	DMA_Cmd(DMA1_Channel1,DISABLE);
	DMA_SetCurrDataCounter(DMA1_Channel1,MYDMA_Size); // 重置计数器
	DMA_Cmd(DMA1_Channel1,ENABLE);
	while(DMA_GetFlagStatus(DMA1_FLAG_TC1) == RESET)
	{
		
	}
	DMA_ClearFlag(DMA1_FLAG_TC1);
}

