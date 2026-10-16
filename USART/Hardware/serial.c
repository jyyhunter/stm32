#include "serial.h"
#include <stdio.h>
#include <stdarg.h>


void Serial_Init()
{
	// 开启USART1的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);
	// 开启GPIO的时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	// 配置引脚模式
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;// 复用推挽输出
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);
	
	
	// 初始化USART
	USART_InitTypeDef USART_InitStrcture;
	USART_InitStrcture.USART_BaudRate = 9600;
	USART_InitStrcture.USART_HardwareFlowControl = USART_HardwareFlowControl_None;// 不使用硬件流控制
	USART_InitStrcture.USART_Mode = USART_Mode_Tx;
	USART_InitStrcture.USART_Parity = USART_Parity_No; // 校验位
	USART_InitStrcture.USART_StopBits = USART_StopBits_1; //一位停止位
	USART_InitStrcture.USART_WordLength = USART_WordLength_8b;
	USART_Init(USART1,&USART_InitStrcture);
	
	
	// 开启中断的方法
	USART_ITConfig(USART1,USART_IT_RXNE,ENABLE);
	// USART1_IRQn:中断方法
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitTypeDef NVIC_InitStructure;
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=1;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	USART_Cmd(USART1,ENABLE);
}


void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1,Byte);// 写入到TDR
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
}

void Serial_SendArray(uint8_t *Array,uint16_t Length)
{
	uint16_t pos;
	for(pos = 0; pos < Length;pos ++)
	{
		Serial_SendByte(Array[pos]);
	}
}
// 发送照片
void Serial_SendString(char * String)
{
	uint8_t i;
	for(i = 0; String[i]!=0; i++)
	{
		Serial_SendByte(String[i]);
	}
}
uint32_t Serial_pow(uint32_t x, uint32_t y)
{
	uint32_t Result = 1;
	while(y--)
	{
		Result *= x;
	}
	return Result;
}

void Serial_SendNumber(uint32_t Number, uint8_t Length)
{
	
	uint8_t i ;
	for(i = 0; i < Length;i++)
	{
		Serial_SendByte(Number / Serial_pow(10,Length - i - 1) % 10 + 0x30);
	}
}

// 将printf重定向到串口
int fpuc(int ch, FILE* f)
{
	Serial_SendByte(ch);
	return ch;
}
// 可变参数的结构
void Serial_Printf(char * format,...)
{
	char String[100];
	va_list arg;
	va_start(arg,format);
	vsprintf(String,format,arg);
	va_end(arg);
	Serial_SendString(String);
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_RXNE) == SET)
	{
		// 读取串口的数据
		
		
		USART_ClearITPendingBit(USART1,USART_IT_RXNE);
	}
}




