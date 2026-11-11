#include "uart_ring.h"
#include <string.h>

void ring_init(ring_buffer_t *r)
{
    r->head = 0;
    r->tail = 0;
}

void ring_clear(ring_buffer_t *r)
{
    r->head = 0;
    r->tail = 0;
}

bool ring_put(ring_buffer_t *r, uint8_t c)
{
    uint16_t next = (uint16_t)((r->head + 1) % RING_SIZE);
    if (next == r->tail) {
        return false;               /* 缓冲区满，丢弃 */
    }
    r->buf[r->head] = c;
    r->head = next;
    return true;
}

bool ring_get(ring_buffer_t *r, uint8_t *c)
{
    if (r->head == r->tail) {
        return false;               /* 空 */
    }
    *c = r->buf[r->tail];
    r->tail = (uint16_t)((r->tail + 1) % RING_SIZE);
    return true;
}

uint16_t ring_available(ring_buffer_t *r)
{
    return (uint16_t)((r->head - r->tail + RING_SIZE) % RING_SIZE);
}

bool ring_peek(ring_buffer_t *r, uint16_t offset, uint8_t *c)
{
    if (offset >= ring_available(r)) {
        return false;
    }
    uint16_t idx = (uint16_t)((r->tail + offset) % RING_SIZE);
    *c = r->buf[idx];
    return true;
}

void ring_discard(ring_buffer_t *r, uint16_t n)
{
    uint16_t avail = ring_available(r);
    if (n > avail) {
        n = avail;
    }
    r->tail = (uint16_t)((r->tail + n) % RING_SIZE);
}

bool ring_find(ring_buffer_t *r, const char *str, uint16_t *idx)
{
    uint16_t avail = ring_available(r);
    uint16_t slen  = (uint16_t)strlen(str);
    if (slen == 0 || slen > avail) {
        return false;
    }

    for (uint16_t i = 0; i <= avail - slen; i++) {
        bool match = true;
        for (uint16_t j = 0; j < slen; j++) {
            uint8_t c;
            ring_peek(r, (uint16_t)(i + j), &c);
            if (c != (uint8_t)str[j]) {
                match = false;
                break;
            }
        }
        if (match) {
            if (idx) {
                *idx = i;
            }
            return true;
        }
    }
    return false;
}
