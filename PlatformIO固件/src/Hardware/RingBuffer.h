#ifndef __RINGBUFFER_H
#define __RINGBUFFER_H

#include <stdint.h>

/* 字节环形缓冲：中断写入、主循环读取，单生产者单消费者无需关中断 */
typedef struct
{
    uint8_t *buffer;
    uint16_t size;                    /* 缓冲区总容量，实际可用 size-1 */
    volatile uint16_t head;           /* 写位置（中断中递增） */
    volatile uint16_t tail;           /* 读位置（主循环递增） */
} RingBuffer_t;

void     RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t size);
uint16_t RingBuffer_Count(const RingBuffer_t *rb);
uint8_t  RingBuffer_IsFull(const RingBuffer_t *rb);
uint8_t  RingBuffer_IsEmpty(const RingBuffer_t *rb);
uint8_t  RingBuffer_Put(RingBuffer_t *rb, uint8_t byte);
uint8_t  RingBuffer_Get(RingBuffer_t *rb, uint8_t *byte);

#endif
