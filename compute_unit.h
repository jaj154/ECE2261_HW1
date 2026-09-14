#ifndef COMPUTE_UNIT_H
#define COMPUTE_UNIT_H

#include "pipeline_types.h"
#include <stdint.h>

#define PREDICT_WINDOW                 8u      /* summary records per prediction */
#define PROJECTION_TICKS               400.0f  /* how far ahead we project the trend */
#define RISING_THRESHOLD_C_PER_TICK    0.01f
#define FALLING_THRESHOLD_C_PER_TICK  (-0.01f)
#define OSCILLATION_SIGN_CHANGES       3u

/* Fits a simple least-squares linear trend over the last
 * PREDICT_WINDOW summary means and classifies the HVAC's apparent
 * behavior. This is the compute-bound stage of the pipeline - small
 * in code, but the natural place a real system would eventually plug
 * in a heavier model (Kalman filter, small neural net, etc.). */
void compute_unit_predict(const summary_record_t *records, uint32_t n, prediction_t *out);

#endif /* COMPUTE_UNIT_H */
