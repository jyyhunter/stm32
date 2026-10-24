#include "RingBuffer.h"

void RingBuffer_Init(RingBuffer_t *rb, uint8_t *buffer, uint16_t size)
{
    rb->buffer = buffer;
    rb->size   = size;
    rb->head   = 0;
    rb->tail   = 0;
}

uint16_t RingBuffer_Count(const RingBuffer_t *rb)
{
    uint16_t head = rb->head;
    uint16_t tail = rb->tail;
    if (head >= tail)
        return (uint16_t)(head - tail);
    return (uint16_t)(rb->size - tail + head);
}

uint8_t RingBuffer_IsFull(const RingBuffer_t *rb)
{
    return (RingBuffer_Count(rb) >= (uint16_t)(rb->size - 1)) ? 1 : 0;
}

uint8_t RingBuffer_IsEmpty(const RingBuffer_t *rb)
{
    return (rb->head == rb->tail) ? 1 : 0;
}

uint8_t RingBuffer_Put(RingBuffer_t *rb, uint8_t byte)
{
    uint16_t next = (uint16_t)((rb->head + 1) % rb->size);
    if (next == rb->tail)
        return 0;                       /* 满，丢弃 */
    rb->buffer[rb->head] = byte;
    rb->head = next;
    return 1;
}

uint8_t RingBuffer_Get(RingBuffer_t *rb, uint8_t *byte)
{
    if (rb->head == rb->tail)
        return 0;                       /* 空 */
    *byte = rb->buffer[rb->tail];
    rb->tail = (uint16_t)((rb->tail + 1) % rb->size);
    return 1;
}
