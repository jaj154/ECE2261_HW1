#include <stdio.h>
#include <math.h>
#include "compute_unit.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); failures++; } \
    else         { printf("  PASS: %s\n", msg); } \
} while (0)

static int approx(float a, float b, float eps)
{
    return fabsf(a - b) <= eps;
}

static summary_record_t make_rec(uint32_t tick_start, uint32_t tick_end, float mean)
{
    summary_record_t r;
    r.tick_start = tick_start;
    r.tick_end = tick_end;
    r.min_c = mean - 0.5f;
    r.max_c = mean + 0.5f;
    r.mean_c = mean;
    r.stddev_c = 0.1f;
    r.samples_in = 16;
    r.samples_kept = 16;
    return r;
}

static void test_zero_records_no_crash(void)
{
    printf("test_zero_records_no_crash\n");
    prediction_t pred;
    compute_unit_predict(NULL, 0, &pred);
    CHECK(pred.tick == 0, "tick defaults to 0 for empty input");
    CHECK(pred.trend_slope_c_per_tick == 0.0f, "slope defaults to 0 for empty input");
    CHECK(pred.state == HVAC_STATE_NOMINAL, "state defaults to NOMINAL for empty input");
}

static void test_flat_trend_is_nominal(void)
{
    printf("test_flat_trend_is_nominal\n");
    summary_record_t recs[PREDICT_WINDOW];
    for (uint32_t i = 0; i < PREDICT_WINDOW; i++) {
        recs[i] = make_rec(i * 16, i * 16 + 15, 22.0f); /* constant mean */
    }
    prediction_t pred;
    compute_unit_predict(recs, PREDICT_WINDOW, &pred);

    CHECK(approx(pred.trend_slope_c_per_tick, 0.0f, 0.001f), "flat data yields ~zero slope");
    CHECK(pred.state == HVAC_STATE_NOMINAL, "flat trend classified as NOMINAL");
    CHECK(approx(pred.projected_temp_c, 22.0f, 0.1f), "projection matches flat mean");
}

static void test_rising_trend_is_undercooling(void)
{
    printf("test_rising_trend_is_undercooling\n");
    summary_record_t recs[PREDICT_WINDOW];
    for (uint32_t i = 0; i < PREDICT_WINDOW; i++) {
        /* Steep, steady rise: well above RISING_THRESHOLD_C_PER_TICK */
        recs[i] = make_rec(i * 16, i * 16 + 15, 20.0f + (float)i * 0.5f);
    }
    prediction_t pred;
    compute_unit_predict(recs, PREDICT_WINDOW, &pred);

    CHECK(pred.trend_slope_c_per_tick > RISING_THRESHOLD_C_PER_TICK, "slope exceeds rising threshold");
    CHECK(pred.state == HVAC_STATE_UNDERCOOLING, "steady rise classified as UNDERCOOLING");
    CHECK(pred.projected_temp_c > recs[PREDICT_WINDOW - 1].mean_c, "projection extrapolates further upward");
}

static void test_falling_trend_is_overcooling(void)
{
    printf("test_falling_trend_is_overcooling\n");
    summary_record_t recs[PREDICT_WINDOW];
    for (uint32_t i = 0; i < PREDICT_WINDOW; i++) {
        recs[i] = make_rec(i * 16, i * 16 + 15, 25.0f - (float)i * 0.5f);
    }
    prediction_t pred;
    compute_unit_predict(recs, PREDICT_WINDOW, &pred);

    CHECK(pred.trend_slope_c_per_tick < FALLING_THRESHOLD_C_PER_TICK, "slope exceeds falling threshold");
    CHECK(pred.state == HVAC_STATE_OVERCOOLING, "steady fall classified as OVERCOOLING");
}

static void test_oscillation_is_short_cycling(void)
{
    printf("test_oscillation_is_short_cycling\n");
    summary_record_t recs[PREDICT_WINDOW];
    float vals[PREDICT_WINDOW] = {22.0f, 23.0f, 21.0f, 23.0f, 21.0f, 23.0f, 21.0f, 22.0f};
    for (uint32_t i = 0; i < PREDICT_WINDOW; i++) {
        recs[i] = make_rec(i * 16, i * 16 + 15, vals[i]);
    }
    prediction_t pred;
    compute_unit_predict(recs, PREDICT_WINDOW, &pred);

    CHECK(pred.state == HVAC_STATE_SHORT_CYCLING, "alternating means classified as SHORT-CYCLING");
}

static void test_single_record_no_crash(void)
{
    printf("test_single_record_no_crash\n");
    summary_record_t recs[1] = { make_rec(0, 15, 22.0f) };
    prediction_t pred;
    compute_unit_predict(recs, 1, &pred);
    /* With n==1, denom is 0 (no spread in x), so slope must fall back
     * to 0 rather than divide by zero. */
    CHECK(pred.trend_slope_c_per_tick == 0.0f, "single-record window falls back to zero slope safely");
    CHECK(pred.state == HVAC_STATE_NOMINAL, "single-record window classified as NOMINAL");
}

int main(void)
{
    printf("=== compute_unit test suite ===\n");
    test_zero_records_no_crash();
    test_flat_trend_is_nominal();
    test_rising_trend_is_undercooling();
    test_falling_trend_is_overcooling();
    test_oscillation_is_short_cycling();
    test_single_record_no_crash();

    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
