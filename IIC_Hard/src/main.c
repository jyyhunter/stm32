/**
  ******************************************************************************
  * @file    main.c
  * @brief   STM32F103VET6 标准外设库工程模板 —— 上电验证例程
  *          现象：板载 LED 每 500ms 翻转一次，USART1 周期打印计数。
  *
  *          开发时：① 改下面 LED 引脚匹配你的板子；
  *                 ② 在 main() 的 while(1) 中添加你的业务代码；
  *                 ③ 新外设驱动可放到 src/ 或 lib/ 下（见 README）。
  ******************************************************************************
  */
#include "stm32f10x.h"
#include <stdio.h>
#include "IIC.h"
/* ============================ 板级引脚配置（按需修改） ============================ */
/* 最小系统板常见板载 LED 在 PC13（低电平点亮）；若你的板子在 PB5 等，改这里即可。 */
#define LED_PORT        GPIOC
#define LED_PIN         GPIO_Pin_13
#define LED_RCC         RCC_APB2Periph_GPIOC
#define LED_ON()        GPIO_ResetBits(LED_PORT, LED_PIN)
#define LED_OFF()       GPIO_SetBits(LED_PORT, LED_PIN)
#define LED_TOGGLE()    GPIO_WriteBit(LED_PORT, LED_PIN, \
                        (BitAction)(1 - GPIO_ReadOutputDataBit(LED_PORT, LED_PIN)))
/* ================================================================================ */

/* 函数声明 */
static void LED_Init(void);
static void USART1_Init(uint32_t baudrate);
static void Delay_us(uint32_t us);
static void Delay_ms(uint32_t ms);

/* printf 的输出重定向在 syscalls.c 的 _write() 中实现（GCC newlib 方式）。 */

int main(void)
{
    /* 系统时钟已由启动文件调用的 SystemInit() 配置为 72MHz（HSE=8MHz） */
    LED_Init();
    USART1_Init(115200);

    printf("\r\n==== STM32F103VET6 SPL Template ====\r\n");

    uint32_t counter = 0;
    while (1)
    {
        /* ===== 在这里写你的主逻辑 ===== */
        LED_TOGGLE();
        printf("running: %lu\r\n", counter++);
        Delay_ms(500);
    }
}

/* ----------------------------- 板级初始化函数 ----------------------------- */

static void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(LED_RCC, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode  = GPIO_Mode_Out_PP;   /* 推挽输出 */
    gpio.GPIO_Pin   = LED_PIN;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(LED_PORT, &gpio);
    LED_OFF();
}

/* USART1: PA9 = TX（复用推挽）, PA10 = RX（浮空输入） */
static void USART1_Init(uint32_t baudrate)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1 | RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode  = GPIO_Mode_AF_PP;     /* PA9 TX 复用推挽 */
    gpio.GPIO_Pin   = GPIO_Pin_9;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &gpio);

    gpio.GPIO_Mode = GPIO_Mode_IN_FLOATING; /* PA10 RX 浮空输入 */
    gpio.GPIO_Pin  = GPIO_Pin_10;
    GPIO_Init(GPIOA, &gpio);

    USART_InitTypeDef usart;
    usart.USART_BaudRate            = baudrate;
    usart.USART_WordLength          = USART_WordLength_8b;
    usart.USART_StopBits            = USART_StopBits_1;
    usart.USART_Parity              = USART_Parity_No;
    usart.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
    usart.USART_Mode                = USART_Mode_Tx | USART_Mode_Rx;
    USART_Init(USART1, &usart);
    USART_Cmd(USART1, ENABLE);
}

/* ----------------------------- 延时（SysTick，AHB/8=9MHz） ----------------------------- */
/* 无需中断，阻塞式；Delay_us 最大约 1860000us */
static void Delay_us(uint32_t us)
{
    SysTick->LOAD = 9 * us;
    SysTick->VAL  = 0;
    SysTick->CTRL = 0x00000005;            /* 时钟源 AHB/8，使能计数 */
    while ((SysTick->CTRL & 0x00010000) == 0) {}
    SysTick->CTRL = 0;
    SysTick->VAL  = 0;
}

static void Delay_ms(uint32_t ms)
{
    while (ms--) {
        Delay_us(1000);
    }
}
