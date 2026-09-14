#ifndef PIPELINE_TYPES_H
#define PIPELINE_TYPES_H

#include <stdint.h>

/* Raw sample captured by the I/O unit at high sample rate. */
typedef struct {
    uint32_t tick;       /* simulated hardware timestamp (systick count) */
    float    value_c;    /* temperature reading, degrees Celsius */
    uint8_t  is_outlier; /* set in-place by the memory unit during filtering */
} raw_sample_t;

/* Compact summary record produced by the memory unit. This is what
 * actually gets kept around / logged for offline analysis instead of
 * every raw sample. */
typedef struct {
    uint32_t tick_start;
    uint32_t tick_end;
    float    min_c;
    float    max_c;
    float    mean_c;
    float    stddev_c;
    uint16_t samples_in;   /* samples fed into this window */
    uint16_t samples_kept; /* samples kept after outlier rejection */
} summary_record_t;

typedef enum {
    HVAC_STATE_NOMINAL = 0,
    HVAC_STATE_UNDERCOOLING,   /* rising trend  - HVAC not keeping up   */
    HVAC_STATE_OVERCOOLING,    /* falling trend - overshoot/overcooling */
    HVAC_STATE_SHORT_CYCLING,  /* oscillation detected                  */
} hvac_state_t;

typedef struct {
    uint32_t     tick;
    float        trend_slope_c_per_tick;
    float        projected_temp_c; /* naive projection N ticks ahead */
    hvac_state_t state;
} prediction_t;

#endif /* PIPELINE_TYPES_H */
