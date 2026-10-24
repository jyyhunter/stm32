#include "AS5600.h"
#include "Delay.h"

/* AS5600 7位地址 0x36 */
#define AS5600_ADDR_WRITE   (0x36 << 1)        /* 0x6C */
#define AS5600_ADDR_READ    ((0x36 << 1) | 1)  /* 0x6D */
#define AS5600_RAW_ANGLE_H  0x0C
#define AS5600_RAW_ANGLE_L  0x0D
#define AS5600_STATUS       0x0B

#define SCL_PORT  GPIOB
#define SCL_PIN   GPIO_Pin_6
#define SDA_PORT  GPIOB
#define SDA_PIN   GPIO_Pin_7

static void SCL_Write(uint8_t v) { if (v) GPIO_SetBits(SCL_PORT, SCL_PIN); else GPIO_ResetBits(SCL_PORT, SCL_PIN); }
static void SDA_Write(uint8_t v) { if (v) GPIO_SetBits(SDA_PORT, SDA_PIN); else GPIO_ResetBits(SDA_PORT, SDA_PIN); }
static uint8_t SDA_Read(void)    { return GPIO_ReadInputDataBit(SDA_PORT, SDA_PIN); }

/* SDA 切换为推挽输出 */
static void SDA_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

/* SDA 切换为上拉输入（释放总线，读取从机应答/数据） */
static void SDA_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SDA_PORT, &GPIO_InitStructure);
}

static void I2C_Delay(void) { Delay_us(2); }

static void I2C_Start(void)
{
    SDA_SetOutput();
    SDA_Write(1); SCL_Write(1); I2C_Delay();
    SDA_Write(0); I2C_Delay();
    SCL_Write(0); I2C_Delay();
}

static void I2C_Stop(void)
{
    SDA_SetOutput();
    SDA_Write(0); SCL_Write(1); I2C_Delay();
    SDA_Write(1); I2C_Delay();
}

/* 返回 0=收到ACK，1=NACK */
static uint8_t I2C_SendByte(uint8_t byte)
{
    uint8_t i, ack;
    SDA_SetOutput();
    for (i = 0; i < 8; i++)
    {
        SCL_Write(0);
        SDA_Write(byte & 0x80);
        byte <<= 1;
        I2C_Delay();
        SCL_Write(1);
        I2C_Delay();
    }
    SCL_Write(0);
    SDA_SetInput();              /* 释放SDA，等待从机拉低 */
    I2C_Delay();
    SCL_Write(1);
    I2C_Delay();
    ack = SDA_Read();            /* 0=ACK */
    SCL_Write(0);
    return ack;
}

/* ack=0 发ACK，ack=1 发NACK */
static uint8_t I2C_ReadByte(uint8_t ack)
{
    uint8_t i, byte = 0;
    SDA_SetInput();
    for (i = 0; i < 8; i++)
    {
        SCL_Write(0);
        I2C_Delay();
        SCL_Write(1);
        byte <<= 1;
        if (SDA_Read()) byte |= 0x01;
        I2C_Delay();
    }
    SCL_Write(0);
    SDA_SetOutput();
    SDA_Write(ack ? 1 : 0);     /* ACK=0(低)，NACK=1(高) */
    I2C_Delay();
    SCL_Write(1);
    I2C_Delay();
    SCL_Write(0);
    SDA_Write(1);
    return byte;
}

void AS5600_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* SCL 推挽输出 */
    GPIO_InitStructure.GPIO_Pin = SCL_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; // 推挽输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCL_PORT, &GPIO_InitStructure);

    SDA_SetOutput();
    SCL_Write(1); SDA_Write(1);
}

uint16_t AS5600_GetRawAngle(void)
{
    uint8_t high, low;
    I2C_Start();
    I2C_SendByte(AS5600_ADDR_WRITE);
    I2C_SendByte(AS5600_RAW_ANGLE_H);
    I2C_Start();
    I2C_SendByte(AS5600_ADDR_READ);
    high = I2C_ReadByte(0);   /* ACK */
    low  = I2C_ReadByte(1);   /* NACK */
    I2C_Stop();
    return (uint16_t)(((high << 8) | low) & 0x0FFF);
}

float AS5600_GetMechAngle(void)
{
    return (float)AS5600_GetRawAngle() * 6.28318530718f / 4096.0f;
}

uint8_t AS5600_ReadReg(uint8_t reg)
{
    uint8_t val;
    I2C_Start();
    I2C_SendByte(AS5600_ADDR_WRITE);
    I2C_SendByte(reg);
    I2C_Start();
    I2C_SendByte(AS5600_ADDR_READ);
    val = I2C_ReadByte(1);   /* NACK */
    I2C_Stop();
    return val;
}

uint8_t AS5600_GetStatus(void)
{
    return AS5600_ReadReg(AS5600_STATUS);
}
