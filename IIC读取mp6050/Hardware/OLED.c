#include "stm32f10x.h"
#include "OLED_Font.h"
#include "Delay.h"

/* ============================================================
 *  0.96 寸 OLED 128x64 SSD1306 软件 I2C 驱动
 *    SCL : PB8   SDA : PB9
 *
 *  本版相对江科大例程的改动（提高接线兼容性）：
 *    1) SCL/SDA 采用推挽输出，读应答时 SDA 切上拉输入，
 *       即使 OLED 模块没有板载上拉电阻也能通信；
 *    2) I2C 时序加入 2us 延时（约 200kHz），兼容较慢的模块；
 *    3) OLED_Init 自动在 0x78 / 0x7A 两个地址间探测，
 *       结果可用 OLED_IsReady() / OLED_GetAddr() 读取。
 * ============================================================ */

#define OLED_SCL_PORT  GPIOB
#define OLED_SCL_PIN   GPIO_Pin_8
#define OLED_SDA_PORT  GPIOB
#define OLED_SDA_PIN   GPIO_Pin_9

static uint8_t OLED_Addr = 0x78;   /* 8位写地址：0x78(0x3C) 或 0x7A(0x3D) */
static uint8_t OLED_Ready = 0;     /* 1=检测到 OLED */

static void OLED_SCL_Write(uint8_t v) { if (v) GPIO_SetBits(OLED_SCL_PORT, OLED_SCL_PIN); else GPIO_ResetBits(OLED_SCL_PORT, OLED_SCL_PIN); }
static void OLED_SDA_Write(uint8_t v) { if (v) GPIO_SetBits(OLED_SDA_PORT, OLED_SDA_PIN); else GPIO_ResetBits(OLED_SDA_PORT, OLED_SDA_PIN); }
static uint8_t OLED_SDA_Read(void)    { return GPIO_ReadInputDataBit(OLED_SDA_PORT, OLED_SDA_PIN); }

/* SDA 推挽输出（发送数据/命令） */
static void OLED_SDA_SetOutput(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = OLED_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OLED_SDA_PORT, &GPIO_InitStructure);
}

/* SDA 上拉输入（释放总线，读取从机应答） */
static void OLED_SDA_SetInput(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Pin = OLED_SDA_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(OLED_SDA_PORT, &GPIO_InitStructure);
}

static void OLED_I2C_Delay(void) { Delay_us(2); }

void OLED_I2C_Init(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

	/* SCL 推挽输出 */
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Pin = OLED_SCL_PIN;
	GPIO_Init(OLED_SCL_PORT, &GPIO_InitStructure);

	/* SDA 推挽输出 */
	OLED_SDA_SetOutput();

	OLED_SCL_Write(1);
	OLED_SDA_Write(1);
}

void OLED_I2C_Start(void)
{
	OLED_SDA_SetOutput();
	OLED_SDA_Write(1);
	OLED_SCL_Write(1);
	OLED_I2C_Delay();
	OLED_SDA_Write(0);
	OLED_I2C_Delay();
	OLED_SCL_Write(0);
	OLED_I2C_Delay();
}

void OLED_I2C_Stop(void)
{
	OLED_SDA_SetOutput();
	OLED_SDA_Write(0);
	OLED_I2C_Delay();
	OLED_SCL_Write(1);
	OLED_I2C_Delay();
	OLED_SDA_Write(1);
	OLED_I2C_Delay();
}

/* 发送一个字节，返回 0=收到ACK，1=无应答 */
static uint8_t OLED_I2C_SendByte(uint8_t Byte)
{
	uint8_t i, ack;
	OLED_SDA_SetOutput();
	for (i = 0; i < 8; i++)
	{
		OLED_SCL_Write(0);
		OLED_SDA_Write(!!(Byte & (0x80 >> i)));
		OLED_I2C_Delay();
		OLED_SCL_Write(1);
		OLED_I2C_Delay();
	}
	OLED_SCL_Write(0);
	OLED_SDA_SetInput();          /* 释放 SDA，读应答 */
	OLED_I2C_Delay();
	OLED_SCL_Write(1);
	OLED_I2C_Delay();
	ack = OLED_SDA_Read();       /* 0=ACK */
	OLED_SCL_Write(0);
	OLED_SDA_SetOutput();
	OLED_SDA_Write(1);
	return ack;
}

/* 探测某 8 位写地址是否有应答，返回 0=有应答 */
static uint8_t OLED_ProbeAddr(uint8_t addr)
{
	uint8_t ack;
	OLED_I2C_Start();
	ack = OLED_I2C_SendByte(addr);
	OLED_I2C_Stop();
	return ack;
}

/* 在 PB8/PB9 上扫描 OLED，返回 0x78 / 0x7A / 0(未找到) */
uint8_t OLED_ScanAddr(void)
{
	OLED_I2C_Init();
	Delay_ms(20);
	if (OLED_ProbeAddr(0x78) == 0)
	{
		return 0x78;
	}
	if (OLED_ProbeAddr(0x7A) == 0)
	{
		return 0x7A;
	}
	return 0;
}

void OLED_WriteCommand(uint8_t Command)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_Addr);
	OLED_I2C_SendByte(0x00);
	OLED_I2C_SendByte(Command);
	OLED_I2C_Stop();
}

void OLED_WriteData(uint8_t Data)
{
	OLED_I2C_Start();
	OLED_I2C_SendByte(OLED_Addr);
	OLED_I2C_SendByte(0x40);
	OLED_I2C_SendByte(Data);
	OLED_I2C_Stop();
}

void OLED_SetCursor(uint8_t Y, uint8_t X)
{
	OLED_WriteCommand(0xB0 | Y);
	OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));
	OLED_WriteCommand(0x00 | (X & 0x0F));
}

void OLED_Clear(void)
{
	uint8_t i, j;
	for (j = 0; j < 8; j++)
	{
		OLED_SetCursor(j, 0);
		for(i = 0; i < 128; i++)
		{
			OLED_WriteData(0x00);
		}
	}
}

void OLED_ShowChar(uint8_t Line, uint8_t Column, char Char)
{
	uint8_t i;
	OLED_SetCursor((Line - 1) * 2, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i]);
	}
	OLED_SetCursor((Line - 1) * 2 + 1, (Column - 1) * 8);
	for (i = 0; i < 8; i++)
	{
		OLED_WriteData(OLED_F8x16[Char - ' '][i + 8]);
	}
}

void OLED_ShowString(uint8_t Line, uint8_t Column, char *String)
{
	uint8_t i;
	for (i = 0; String[i] != '\0'; i++)
	{
		OLED_ShowChar(Line, Column + i, String[i]);
	}
}

uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
	uint32_t Result = 1;
	while (Y--)
	{
		Result *= X;
	}
	return Result;
}

void OLED_ShowNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

void OLED_ShowSignedNum(uint8_t Line, uint8_t Column, int32_t Number, uint8_t Length)
{
	uint8_t i;
	uint32_t Number1;
	if (Number >= 0)
	{
		OLED_ShowChar(Line, Column, '+');
		Number1 = Number;
	}
	else
	{
		OLED_ShowChar(Line, Column, '-');
		Number1 = -Number;
	}
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i + 1, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0');
	}
}

void OLED_ShowHexNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i, SingleNumber;
	for (i = 0; i < Length; i++)
	{
		SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
		if (SingleNumber < 10)
		{
			OLED_ShowChar(Line, Column + i, SingleNumber + '0');
		}
		else
		{
			OLED_ShowChar(Line, Column + i, SingleNumber - 10 + 'A');
		}
	}
}

void OLED_ShowBinNum(uint8_t Line, uint8_t Column, uint32_t Number, uint8_t Length)
{
	uint8_t i;
	for (i = 0; i < Length; i++)
	{
		OLED_ShowChar(Line, Column + i, Number / OLED_Pow(2, Length - i - 1) % 2 + '0');
	}
}

void OLED_Init(void)
{
	OLED_Addr = OLED_ScanAddr();   /* 上电稳定 + 自动探测地址 */
	if (OLED_Addr != 0)
	{
		OLED_Ready = 1;
	}
	else
	{
		OLED_Addr = 0x78;          /* 未探测到也按 0x78 继续，便于后续重试 */
		OLED_Ready = 0;
	}

	OLED_WriteCommand(0xAE);
	OLED_WriteCommand(0xD5);
	OLED_WriteCommand(0x80);
	OLED_WriteCommand(0xA8);
	OLED_WriteCommand(0x3F);
	OLED_WriteCommand(0xD3);
	OLED_WriteCommand(0x00);
	OLED_WriteCommand(0x40);
	OLED_WriteCommand(0xA1);
	OLED_WriteCommand(0xC8);
	OLED_WriteCommand(0xDA);
	OLED_WriteCommand(0x12);
	OLED_WriteCommand(0x81);
	OLED_WriteCommand(0xCF);
	OLED_WriteCommand(0xD9);
	OLED_WriteCommand(0xF1);
	OLED_WriteCommand(0xDB);
	OLED_WriteCommand(0x30);
	OLED_WriteCommand(0xA4);
	OLED_WriteCommand(0xA6);
	OLED_WriteCommand(0x8D);
	OLED_WriteCommand(0x14);
	OLED_WriteCommand(0xAF);

	OLED_Clear();
}

uint8_t OLED_IsReady(void) { return OLED_Ready; }
uint8_t OLED_GetAddr(void) { return OLED_Addr; }
