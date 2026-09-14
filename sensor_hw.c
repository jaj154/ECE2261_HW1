#include "sensor_hw.h"
#include <math.h>

/*
 * On real Raspberry Pi 5 bare-metal firmware this file is where the
 * memory-mapped peripheral access would live, e.g. something like:
 *
 *   #define TEMP_ADC_BASE  0xFE215000u
 *   #define TEMP_ADC_DATA  (*(volatile uint32_t *)(TEMP_ADC_BASE + 0x08))
 *
 *   float sensor_hw_read_raw(uint32_t tick) {
 *       (void)tick;
 *       return adc_counts_to_celsius(TEMP_ADC_DATA);
 *   }
 *
 * Since this build is a desktop stand-in for the bare-metal target
 * (per Geeny IDE constraints), we synthesize a plausible machine-room
 * temperature signal instead: a slow drift representing the HVAC duty
 * cycle, ambient sensor jitter, and occasional glitches/outliers that
 * the memory unit is responsible for rejecting.
 */

static uint32_t s_seed = 12345u;

static float pseudo_rand_unit(void)
{
    /* Small LCG so the "sensor" is reproducible run-to-run without
     * depending on libc rand() global state. */
    s_seed = s_seed * 1103515245u + 12345u;
    return (float)((s_seed >> 16) & 0x7FFFu) / 32768.0f; /* [0,1) */
}

void sensor_hw_init(void)
{
    s_seed = 12345u;
}

float sensor_hw_read_raw(uint32_t tick)
{
    float baseline   = 22.0f;
    float duty_cycle = 1.5f * sinf((float)tick / 400.0f);
    float jitter     = (pseudo_rand_unit() - 0.5f) * 0.4f;

    float value = baseline + duty_cycle + jitter;

    /* ~1% chance of an ADC glitch / EMI spike per sample. */
    if (pseudo_rand_unit() < 0.01f) {
        value += (pseudo_rand_unit() - 0.5f) * 20.0f;
    }

    return value;
}
