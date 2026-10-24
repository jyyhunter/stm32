#include "GPIO_TEST.h"

void GPIO_InitT()
{
    GPIO_InitTypeDef GPIO_InitStructure;
    // 使能 GPIOA 时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    // 配置 PA4、PA5、PA6、PA7 为推挽输出
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  // 上拉输入具备驱动能力
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    

    //GPIO_WriteBit(GPIOB, GPIO_Pin_7, 0); // 设置默认电平为高
}