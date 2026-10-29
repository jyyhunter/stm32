#include "IIC.h"

/* MPU6050：7位地址 0x68（AD0 接地），标准库 Send7bitAddress 内部会自动左移 */
#define MPU6050_ADDRESS    0x68
/* 电源管理寄存器1（PWR_MGMT_1） */
#define MPU6050_PWR_MGMT_1 0x6B


// 硬件生成IIC初始化函数
void IIC_Init(void)
{
    /* 1. 使能 GPIOB 与 I2C1 时钟（原代码漏了 I2C1 外设时钟） */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_I2C1, ENABLE); // 在总线1中关在IIC

    /* 2. PB6 = I2C1_SCL, PB7 = I2C1_SDA，复用开漏输出 */
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin   = GPIO_Pin_6 | GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_AF_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* 3. I2C1 配置，标准模式 100kHz */
    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode                = I2C_Mode_I2C;
    I2C_InitStructure.I2C_ClockSpeed          = 100000;
    I2C_InitStructure.I2C_DutyCycle           = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1         = 0x00;
    I2C_InitStructure.I2C_Ack                 = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C1, &I2C_InitStructure);

    I2C_Cmd(I2C1, ENABLE);
    I2C_AcknowledgeConfig(I2C1, ENABLE);
}

/* 向 MPU6050 指定寄存器写一个字节 */
void MPU_WriteReg(uint8_t reg, uint8_t data)
{
    I2C_GenerateSTART(I2C1, ENABLE);                                   /* 起始 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) == ERROR) {}

    I2C_Send7bitAddress(I2C1, MPU6050_ADDRESS, I2C_Direction_Transmitter); /* 设备地址+写 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == ERROR) {}

    I2C_SendData(I2C1, reg);                                          /* 寄存器地址 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING) == ERROR) {}

    I2C_SendData(I2C1, data);                                         /* 数据 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTED) == ERROR) {}

    I2C_GenerateSTOP(I2C1, ENABLE);                                   /* 停止 */
}

void MPU_ReadRegs(uint8_t reg, uint8_t *buf, uint8_t len)
{
    I2C_GenerateSTART(I2C1, ENABLE);                                   /* 起始 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) == ERROR) {}

    I2C_Send7bitAddress(I2C1, MPU6050_ADDRESS, I2C_Direction_Transmitter); /* 设备地址+写 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED) == ERROR) {}

    I2C_SendData(I2C1, reg);                                          /* 寄存器地址 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_TRANSMITTING) == ERROR) {}

    I2C_GenerateSTART(I2C1, ENABLE);                                   /* 重启 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_MODE_SELECT) == ERROR) {}

    I2C_Send7bitAddress(I2C1, MPU6050_ADDRESS, I2C_Direction_Receiver); /* 设备地址+读 */
    while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_RECEIVER_MODE_SELECTED) == ERROR) {}

    for (uint8_t i = 0; i < len; i++)
    {
        if (i == len - 1)
        {
            I2C_AcknowledgeConfig(I2C1, DISABLE);                      /* 最后一个字节 NACK */
            I2C_GenerateSTOP(I2C1, ENABLE);                            /* 停止 */
        }

        while (I2C_CheckEvent(I2C1, I2C_EVENT_MASTER_BYTE_RECEIVED) == ERROR) {}
        buf[i] = I2C_ReceiveData(I2C1);
    }
    I2C_AcknowledgeConfig(I2C1, ENABLE);                               /* 恢复 ACK */
}


