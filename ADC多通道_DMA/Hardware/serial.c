#include "serial.h"

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
	
	USART_Cmd(USART1,ENABLE);
	
	
	
	
}


void Serial_SendByte(uint8_t Byte)
{
	USART_SendData(USART1,Byte);// 写入到TDR
	while(USART_GetFlagStatus(USART1,USART_FLAG_TXE) == RESET);
	
}