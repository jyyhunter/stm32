#include "MYSPI.h"

#define GPIO_SetBit(BitValue) GPIO_WaiteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue)

// SS片选引脚，使能
void MYSPI_W_SS(uint8_t BitValue)
{
    GPIO_WaiteBit(GPIOA, GPIO_Pin_4, (BitAction)BitValue);
}

// SCK时钟引脚，输出
void MYSPI_W_SCK(uint8_t BitValue)
{
    GPIO_WaiteBit(GPIOA, GPIO_Pin_5, (BitAction)BitValue);
}

// SPI写MOSI引脚电平
void MYSPI_W_MOSI(uint8_t BitValue)
{
    GPIO_WaiteBit(GPIOA, GPIO_Pin_7, (BitAction)BitValue);
}

// 读取 MISO引脚电平
uint8_t MYSPI_R_MISO(void)
{
    return GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_6);
}

// 初始化 SPI 引脚
void MYSPI_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    // 配置 SCK、MOSI、SS 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 MISO 为浮空输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 设置默认电平
    MYSPI_W_SS(1);   // 片选高电平
    MYSPI_W_SCK(0);  // 时钟低电平
}


// 使能片选
void mySPI_Start(void)
{
    MYSPI_W_SS(0); // 片选低电平，开始通信
}

void mySPI_Stop(void)
{
    MYSPI_W_SS(1); // 片选高电平，结束通信
}

// 读取数据
uint8_t mySPI_SwapByte(uint8_t byteSend)
{
    uint8_t byteReceive = 0;

    for (int i = 0; i < 8; i++)
    {
        // 发送数据位
        if (byteSend & 0x80) // 检查最高位
            MYSPI_W_MOSI(1);
        else
            MYSPI_W_MOSI(0);

        // 拉高时钟，数据在上升沿被采样
        MYSPI_W_SCK(1);

        // 读取接收数据位
        byteReceive <<= 1; // 左移一位，为接收新位腾出空间
        if (MYSPI_R_MISO())
            byteReceive |= 0x01; // 设置最低位为1

        // 拉低时钟，准备发送下一位
        MYSPI_W_SCK(0);

        byteSend <<= 1; // 左移发送数据，为下一位准备
    }

    return byteReceive;
}