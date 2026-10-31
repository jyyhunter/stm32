#include "Serial.h"
#include <stdio.h>
#include <string.h>

#define SERIAL_RX_SIZE  64U
#define SERIAL_TX_SIZE  256U

static volatile uint8_t  rx_buffer[SERIAL_RX_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;
static volatile uint8_t  tx_buffer[SERIAL_TX_SIZE];
static volatile uint16_t tx_head = 0;
static volatile uint16_t tx_tail = 0;

void Serial_Init(uint32_t baud)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);

    /* PB10 TX 复用推挽 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* PB11 RX 浮空输入 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    USART_InitStructure.USART_BaudRate = baud;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART3, &USART_InitStructure);

    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);
}

void Serial_Write(const uint8_t *data, uint16_t length)
{
    uint16_t i;
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    for (i = 0; i < length; i++)
    {
        uint16_t next = (tx_head + 1U) % SERIAL_TX_SIZE;
        if (next == tx_tail)
            break;
        tx_buffer[tx_head] = data[i];
        tx_head = next;
    }

    if (tx_head != tx_tail)
        USART_ITConfig(USART3, USART_IT_TXE, ENABLE);

    if (!primask)
        __enable_irq();
}

void Serial_WriteString(const char *text)
{
    Serial_Write((const uint8_t *)text, (uint16_t)strlen(text));
}

uint8_t Serial_ReadByte(uint8_t *value)
{
    if (rx_head == rx_tail)
        return 0;

    *value = rx_buffer[rx_tail];
    rx_tail = (rx_tail + 1U) % SERIAL_RX_SIZE;
    return 1;
}

void USART3_IRQHandler(void)
{
    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t value = (uint8_t)USART_ReceiveData(USART3);
        uint16_t next = (rx_head + 1U) % SERIAL_RX_SIZE;
        if (next != rx_tail)
        {
            rx_buffer[rx_head] = value;
            rx_head = next;
        }
    }

    if (USART_GetITStatus(USART3, USART_IT_TXE) != RESET)
    {
        if (tx_tail != tx_head)
        {
            USART_SendData(USART3, tx_buffer[tx_tail]);
            tx_tail = (tx_tail + 1U) % SERIAL_TX_SIZE;
        }
        else
        {
            USART_ITConfig(USART3, USART_IT_TXE, DISABLE);
        }
    }
}

/* printf 重定向到 USART3，使用非阻塞发送缓冲区 */
int fputc(int ch, FILE *f)
{
    uint8_t value = (uint8_t)ch;
    Serial_Write(&value, 1);
    return ch;
}
