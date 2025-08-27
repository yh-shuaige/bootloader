#include <stdlib.h>
#include <string.h>
#include "ringbuffer8.h"


#define rbb_len         rb->length
#define rbb_idx(x)      (uint8_t *)rbb_buff + rbb_size * (x)
#define dat_idx(d, x)   (uint8_t *)(d) + rbb_size * (x)


struct ringbuffer8
{
    uint32_t tail;
    uint32_t head;
    uint32_t length;

    uint8_t buffer[];
};


ringbuffer8_t rb8_new(uint8_t *buff, uint32_t length)
{
    ringbuffer8_t rb = (ringbuffer8_t)buff;
    rb->length = length - sizeof(struct ringbuffer8);

    return rb;
}

static inline uint16_t next_head(ringbuffer8_t rb)
{
    return rb->head + 1 < rbb_len ? rb->head + 1 : 0;
}

static inline uint16_t next_tail(ringbuffer8_t rb)
{
    return rb->tail + 1 < rbb_len ? rb->tail + 1 : 0;
}

bool rb8_empty(ringbuffer8_t rb)
{
    return rb->head == rb->tail;
}

bool rb8_full(ringbuffer8_t rb)
{
    return next_head(rb) == rb->tail;
}

bool rb8_put(ringbuffer8_t rb, uint8_t data)
{
    if (next_head(rb) == rb->tail)
        return false;

    rb->buffer[rb->head] = data;
    rb->head = next_head(rb);

    return true;
}

bool rb8_puts(ringbuffer8_t rb, uint8_t *data, uint32_t size)
{
    bool ret = true;

    for (uint16_t i = 0; i < size && ret; i++)
    {
        ret = rb8_put(rb, data[i]);
    }

    return ret;
}

bool rb8_get(ringbuffer8_t rb, uint8_t *data)
{
    if (rb->head == rb->tail)
        return false;

    *data = rb->buffer[rb->tail];
    rb->tail = next_tail(rb);

    return true;
}

bool rb8_gets(ringbuffer8_t rb, uint8_t *data, uint32_t size)
{
    bool ret = true;

    for (uint16_t i = 0; i < size && ret; i++)
    {
        ret = rb8_get(rb, &data[i]);
    }

    return ret;
}
