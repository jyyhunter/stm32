#include "stm32f10x.h"                  // Device header
#include "Delay.h"
#include "Serial.h"


// 外设位带基址
#define PERIPH_BITBAND_BASE  0x42000000UL
#define PERIPH_REG_BASE      0x40000000UL
// 输入：reg_addr 寄存器地址，bit 第几位
#define BITBAND_PERIPH(reg_addr, bit)  \
(*(volatile uint32_t *)( PERIPH_BITBAND_BASE + ((reg_addr) - PERIPH_REG_BASE)*32UL + (bit)*4UL ))


// 独立看门狗服务、窗口看门狗能够实现硬件故障的条件下的设备复位等；

int main(void)
{
    // ODR决定上拉还是下拉
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB,&GPIO_InitStructure);
    // PA 0 推挽输出  PA 8输入，最难的是对寄存器的操作，什么时机操作什么样的寄存器，需要严格分析
    // 通过寄存器进行操作，是否效率会更高呢
    // GPIOA->CRL = 0x33333333;
    // GPIOA->CRH = 0x44444444;
    // GPIOA->BSRR = 0x00000000;
    // 位绑定实现地址的映射，AFIO是否基于该机制？

    // 操作寄存器会直接带来高效的数据访问
    // GPIOA_BASE + ODR偏移地址 ---- > 寄存器地址
    
    while (1)
    {
       
        
    }
}
