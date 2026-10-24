#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"
#include "RingBuffer.h"
#include "MS901_UART.h"
#include "atk_ms901m.h"
#include "app_protocol.h"
#include "stm32f10x_usart.h"

/*
 * 硬件连接：
 *   ATK-MS901M 模块：TX->PB11(USART3_RX)  RX->PB10(USART3_TX)  VCC/GND
 *   USB-TTL 上位机  ：RXD->PA9(USART1_TX)  TXD->PA10(USART1_RX) GND
 *   波特率均为 115200，8N1。
 */
int main(void)
{
    atk_ms901m_data_t imu;
    uint8_t byte;

    Serial_Init();                  // 上位机串口 USART1（内部同时配置 NVIC 分组2）
    MS901_UART_Init(115200);        // 模块串口 USART3，中断接收写入环形缓冲
    atk_ms901m_init(&imu);

    while (1)
    {
        /* 取出环形缓冲中的字节，逐字节解析；完成一帧姿态角即打包发给上位机 */
        while (RingBuffer_Get(&g_ms901_rx, &byte))
        {
            uint8_t valid = atk_ms901m_parse_byte(&imu, byte);
            if (valid & ATK_MS901M_VALID_ATTITUDE)
            {
                app_protocol_send(&imu);
            }
        }
    }
}
