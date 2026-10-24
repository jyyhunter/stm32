#include "MPU6050.h"
#include "Delay.h"
#define SCL_Port GPIOB
#define SCL_PIN GPIO_Pin_6
#define SDA_Port GPIOB
#define SDA_PIN GPIO_Pin_7

#define W_SCL(x) GPIO_WriteBit(SCL_Port,SCL_PIN,(BitAction)(x))
#define W_SDA(x) GPIO_WriteBit(SDA_Port,SDA_PIN,(BitAction)(x))

#define MPU_ADDR 0x6C
#define	MPU6050_SMPLRT_DIV		0x19
#define	MPU6050_CONFIG			0x1A
#define	MPU6050_GYRO_CONFIG		0x1B
#define	MPU6050_ACCEL_CONFIG	0x1C

#define	MPU6050_ACCEL_XOUT_H	0x3B
#define	MPU6050_ACCEL_XOUT_L	0x3C
#define	MPU6050_ACCEL_YOUT_H	0x3D
#define	MPU6050_ACCEL_YOUT_L	0x3E
#define	MPU6050_ACCEL_ZOUT_H	0x3F
#define	MPU6050_ACCEL_ZOUT_L	0x40
#define	MPU6050_TEMP_OUT_H		0x41
#define	MPU6050_TEMP_OUT_L		0x42
#define	MPU6050_GYRO_XOUT_H		0x43
#define	MPU6050_GYRO_XOUT_L		0x44
#define	MPU6050_GYRO_YOUT_H		0x45
#define	MPU6050_GYRO_YOUT_L		0x46
#define	MPU6050_GYRO_ZOUT_H		0x47
#define	MPU6050_GYRO_ZOUT_L		0x48

#define	MPU6050_PWR_MGMT_1		0x6B
#define	MPU6050_PWR_MGMT_2		0x6C
#define	MPU6050_WHO_AM_I		0x75
/// iic通信
void IIC_Init()
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Pin = SCL_PIN | SDA_PIN;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD; // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(SCL_Port,&GPIO_InitStructure);

    W_SCL(1);
    W_SDA(1);
}

void IIC_W_SCL(uint8_t BitValue)
{
    W_SCL(BitValue);
    Delay_us(10);
}
void IIC_W_SDA(uint8_t BitValue)
{
    W_SDA(BitValue);
    Delay_us(10);
}

uint8_t IIC_R_SDA()
{
    uint8_t BitValue;
    BitAction bit = GPIO_ReadInputDataBit(SDA_Port,SDA_PIN);
    BitValue = (uint8_t)bit;
    Delay_us(10);
    return BitValue;
}
// 开启IIC通信，先拉高SDA和SCL线，然后拉低SDA线，最后拉低SCL线
void IIC_Start()
{
    IIC_W_SDA(1);
    IIC_W_SCL(1);
    IIC_W_SDA(0);
    IIC_W_SCL(0);

}
// 先拉低SDA线，然后拉低SCL线，最后拉高SDA线，最后拉高SCL线
void IIC_Stop()
{
    IIC_W_SDA(0);
    IIC_W_SCL(1);
    IIC_W_SDA(1);
}
// 发送一个字节数据
void IIC_SendByte(uint8_t SendByte)
{
   uint8_t i;
   for(i=0;i<8;i++)
   {
       IIC_W_SDA(SendByte&(0x80 >> i));
       IIC_W_SCL(1);
       IIC_W_SCL(0);
   }
    
}
// 接受一个字节数据
uint8_t IIC_ReceiveByte()
{
    uint8_t Byte = 0x00;
    uint8_t i;
    IIC_W_SDA(1);// 释放SDA线
    for(i=0;i<8;i++)
    {
        IIC_W_SCL(1);
        if(IIC_R_SDA() == 1 )
        {
            Byte |= (0x80 >> i);
        }
        IIC_W_SCL(0);
    }   
    return Byte;
}

void IIC_SendAck(uint8_t AckBit)
{
    IIC_W_SDA(AckBit);
    IIC_W_SCL(1);
    IIC_W_SCL(0);
}
uint8_t IIC_ReceiveAck()
{
    uint8_t AckBit;
    IIC_W_SDA(1); // 释放SDA线
    IIC_W_SCL(1);
    AckBit = IIC_R_SDA();
    IIC_W_SCL(0);
    return AckBit;
}

// MPU指定地址写入数据
void MPU_WriteReg(uint8_t reg,uint8_t data)
{
    IIC_Start();
    IIC_SendByte(MPU_ADDR); // 发送器件地址+写位
    IIC_ReceiveAck();
    IIC_SendByte(reg); // 发送寄存器地址
    IIC_ReceiveAck();
    IIC_SendByte(data); // 发送数据
    IIC_ReceiveAck();
    IIC_Stop();
}
// 读取MPU指定地址的数据
uint8_t MPU_ReadReg(uint8_t reg)
{
    uint8_t data;
    IIC_Start();
    IIC_SendByte(MPU_ADDR); // 发送器件地址+写位
    IIC_ReceiveAck();
    IIC_SendByte(reg); // 发送寄存器地址
    IIC_ReceiveAck();
    IIC_Start(); // 重启总线
    IIC_SendByte(MPU_ADDR | 0x01); // 发送器件地址+读位，修改读写位0--->1
    IIC_ReceiveAck();
    data = IIC_ReceiveByte(); // 接收数据
    IIC_SendAck(1); // 发送NACK，表示不再接收数据,给0表示继续读取数据
    IIC_Stop();
    return data;
}

void MPU_Init()
{
    IIC_Init();
    MPU_WriteReg(MPU6050_PWR_MGMT_1,0x01); // 解除睡眠
    MPU_WriteReg(MPU6050_PWR_MGMT_2,0x00); // 使能所有轴
    MPU_WriteReg(MPU6050_SMPLRT_DIV,0x09); // 设置采样率
    MPU_WriteReg(MPU6050_CONFIG,0x06); // 设置低通滤波器
    MPU_WriteReg(MPU6050_GYRO_CONFIG,0x18); // 设置陀螺仪量程为±2000°/s
    MPU_WriteReg(MPU6050_ACCEL_CONFIG,0x18); // 设置加速度计量程为±2g
}
// 读取MPU6050的加速度数据
void MPU_GetAccelData(int16_t *accelX, int16_t *accelY, int16_t *accelZ,  int16_t *gyroX, int16_t *gyroY, int16_t *gyroZ)
{
    uint8_t highByte, lowByte;
    highByte = MPU_ReadReg(MPU6050_ACCEL_XOUT_H);
    lowByte = MPU_ReadReg(MPU6050_ACCEL_XOUT_L);
    *accelX = (int16_t)((highByte << 8) | lowByte);

    highByte = MPU_ReadReg(MPU6050_ACCEL_YOUT_H);
    lowByte = MPU_ReadReg(MPU6050_ACCEL_YOUT_L);
    *accelY = (int16_t)((highByte << 8) | lowByte);

    highByte = MPU_ReadReg(MPU6050_ACCEL_ZOUT_H);
    lowByte = MPU_ReadReg(MPU6050_ACCEL_ZOUT_L);
    *accelZ = (int16_t)((highByte << 8) | lowByte);

    highByte = MPU_ReadReg(MPU6050_GYRO_XOUT_H);
    lowByte = MPU_ReadReg(MPU6050_GYRO_XOUT_L);
    *gyroX = (int16_t)((highByte << 8) | lowByte);

    highByte = MPU_ReadReg(MPU6050_GYRO_YOUT_H);
    lowByte = MPU_ReadReg(MPU6050_GYRO_YOUT_L);
    *gyroY = (int16_t)((highByte << 8) | lowByte);

    highByte = MPU_ReadReg(MPU6050_GYRO_ZOUT_H);
    lowByte = MPU_ReadReg(MPU6050_GYRO_ZOUT_L);
    *gyroZ = (int16_t)((highByte << 8) | lowByte);
}
uint8_t MPU_GetWhoAmI()
{
    return MPU_ReadReg(MPU6050_WHO_AM_I);
}
