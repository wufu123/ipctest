#ifndef __UART_RING_H
#define __UART_RING_H

#include <stdint.h>
#include <stdbool.h>

#define RING_SIZE 512

typedef struct {
    uint8_t  buf[RING_SIZE];
    volatile uint16_t head;
    volatile uint16_t tail;
} ring_buffer_t;

void     ring_init(ring_buffer_t *r);
void     ring_clear(ring_buffer_t *r);
bool     ring_put(ring_buffer_t *r, uint8_t c);
bool     ring_get(ring_buffer_t *r, uint8_t *c);
uint16_t ring_available(ring_buffer_t *r);
bool     ring_peek(ring_buffer_t *r, uint16_t offset, uint8_t *c);
void     ring_discard(ring_buffer_t *r, uint16_t n);
/* 在缓冲区中查找子串，返回相对 tail 的偏移 */
bool     ring_find(ring_buffer_t *r, const char *str, uint16_t *idx);

#endif /* __UART_RING_H */
