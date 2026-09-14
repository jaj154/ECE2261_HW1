#ifndef MEMORY_UNIT_H
#define MEMORY_UNIT_H

#include "pipeline_types.h"
#include <stdint.h>

#define SUMMARY_RING_SIZE    32u
#define OUTLIER_THRESHOLD_C  3.0f /* fixed deviation-from-mean outlier filter */

typedef struct {
    summary_record_t buf[SUMMARY_RING_SIZE];
    uint32_t          head;
    uint32_t          count;
} summary_ring_t;

void memory_unit_init(summary_ring_t *ring);

/* Consumes a batch of raw samples: rejects outliers, computes
 * min/max/mean/stddev over the survivors, and pushes one compact
 * summary record. This is the "optimize + summarize for offline
 * analysis" stage - trading CPU cycles now for storage later. Marks
 * samples[i].is_outlier in place so callers can inspect what was
 * rejected. */
void memory_unit_process(summary_ring_t *ring, raw_sample_t *samples, uint32_t n);

uint32_t memory_unit_drain(summary_ring_t *ring, summary_record_t *out, uint32_t max_out);

#endif /* MEMORY_UNIT_H */
