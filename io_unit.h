#ifndef IO_UNIT_H
#define IO_UNIT_H

#include "pipeline_types.h"
#include <stdint.h>

#define RAW_RING_SIZE 64u

typedef struct {
    raw_sample_t buf[RAW_RING_SIZE];
    uint32_t     head;  /* next write index */
    uint32_t     count; /* number of unread samples currently buffered */
} raw_ring_t;

void io_unit_init(raw_ring_t *ring);

/* Simulates one high-rate ADC poll. Bare-metal analog: this would
 * normally be invoked from a SysTick/timer ISR, or polled tightly in
 * the superloop. Pushes one sample into the ring buffer. */
void io_unit_sample(raw_ring_t *ring, uint32_t tick);

/* Pops up to max_out samples into caller-provided storage. Returns
 * the number actually popped. Used by the memory unit as consumer. */
uint32_t io_unit_drain(raw_ring_t *ring, raw_sample_t *out, uint32_t max_out);

#endif /* IO_UNIT_H */
