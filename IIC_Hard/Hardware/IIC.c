#include "IIC.h"

#define RegAddress 0xD0 // MPU6050 I2C 地址（写操作）
#define MPU6050_ADDRESS 0x6B // MPU6050 电源管理寄存器地址
void IIC_Init(void)
{
    /* 这里添加 IIC 初始化代码 */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE); // 使能 GPIOB 时钟
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7; // PB6 -> SCL, PB7 -> SDA
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_OD; // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);  

    I2C_InitTypeDef I2C_InitStructure;
    I2C_InitStructure.I2C_Mode = I2C_Mode_I2C;
    I2C_InitStructure.I2C_ClockSpeed = 50000; // 100kHz
    I2C_InitStructure.I2C_DutyCycle = I2C_DutyCycle_2;
    I2C_InitStructure.I2C_OwnAddress1 = 0x00; // 主机地址
    I2C_InitStructure.I2C_Ack = I2C_Ack_Enable;
    I2C_InitStructure.I2C_AcknowledgedAddress = I2C_AcknowledgedAddress_7bit;
    I2C_Init(I2C2, &I2C_InitStructure);
    I2C_Cmd(I2C2, ENABLE); // 使能 I2
    
}
// 写入数据到 MPU6050 寄存器
void MPU_WriteReg(uint8_t reg, uint8_t data)
{
    I2C_GenerateSTART(I2C2, ENABLE); // 发送起始信号
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_MODE_SELECT)); // 等待EV5事件，等待起始信号发送完成

    I2C_Send7bitAddress(I2C2, MPU6050_ADDRESS, I2C_Direction_Transmitter); // 发送设备地址（写操作）
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_TRANSMITTER_MODE_SELECTED)); // 等待地址发送完成

    I2C_SendData(I2C2, RegAddress); // 发送寄存器地址
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTING)); // 等待数据发送完成

    I2C_SendData(I2C2, data); // 发送数据
    while (!I2C_CheckEvent(I2C2, I2C_EVENT_MASTER_BYTE_TRANSMITTED)); // 等待数据发送完成

    I2C_GenerateSTOP(I2C2, ENABLE); // 发送停止信号
}
