#include "stm32f10x.h"
#include "MS901_UART.h"

static uint8_t s_rx_buf[MS901_RXBUF_SIZE];

RingBuffer_t g_ms901_rx;

/**
 * 初始化与 ATK-MS901M 模块连接的 USART3（PB10/PB11，中断接收）
 * 说明：NVIC 分组在 Serial_Init() 中已统一配置为 Group2，请保证先调用 Serial_Init()
 */
void MS901_UART_Init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    /* 开启时钟：USART3 在 APB1，GPIOB 在 APB2 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* PB10 = USART3_TX，复用推挽输出 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP; // 复用推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* PB11 = USART3_RX，上拉输入 */
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    /* USART3 参数：波特率自定义，8 数据位、1 停止位、无校验、收发都使能 */
    USART_InitStructure.USART_BaudRate = baudrate;
    USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    USART_InitStructure.USART_Mode = USART_Mode_Tx | USART_Mode_Rx;
    USART_InitStructure.USART_Parity = USART_Parity_No;
    USART_InitStructure.USART_StopBits = USART_StopBits_1;
    USART_InitStructure.USART_WordLength = USART_WordLength_8b;
    USART_Init(USART3, &USART_InitStructure);

    /* 使能接收非空中断 */
    USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);

    /* NVIC 配置（分组已由 Serial_Init 设为 Group2） */
    NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;
    NVIC_Init(&NVIC_InitStructure);

    USART_Cmd(USART3, ENABLE);

    RingBuffer_Init(&g_ms901_rx, s_rx_buf, MS901_RXBUF_SIZE);
}

/**
 * USART3 中断服务函数：逐字节接收模块数据并写入环形缓冲
 */
void USART3_IRQHandler(void)
{
    /* 处理溢出错误（ORE）：读 SR 后读 DR 即可清除，丢弃该帧等待重同步 */
    if (USART_GetFlagStatus(USART3, USART_FLAG_ORE) != RESET)
    {
        (void)USART_ReceiveData(USART3);
        return;
    }

    if (USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
    {
        uint8_t b = (uint8_t)USART_ReceiveData(USART3);
        RingBuffer_Put(&g_ms901_rx, b);
    }
}
