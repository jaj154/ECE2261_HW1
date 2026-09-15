#include <stdio.h>
#include <math.h>
#include "memory_unit.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); failures++; } \
    else         { printf("  PASS: %s\n", msg); } \
} while (0)

static int approx(float a, float b, float eps)
{
    return fabsf(a - b) <= eps;
}

static raw_sample_t make_sample(uint32_t tick, float v)
{
    raw_sample_t s;
    s.tick = tick;
    s.value_c = v;
    s.is_outlier = 0;
    return s;
}

static void test_init_resets_state(void)
{
    printf("test_init_resets_state\n");
    summary_ring_t ring;
    ring.head = 3; ring.count = 3; /* dirty state */
    memory_unit_init(&ring);
    CHECK(ring.head == 0, "head reset to 0 after init");
    CHECK(ring.count == 0, "count reset to 0 after init");
}

static void test_basic_stats_no_outliers(void)
{
    printf("test_basic_stats_no_outliers\n");
    summary_ring_t ring;
    memory_unit_init(&ring);

    /* Uniform batch: 20, 21, 22, 23, 24 -> mean 22, min 20, max 24 */
    raw_sample_t batch[5] = {
        make_sample(0, 20.0f), make_sample(1, 21.0f), make_sample(2, 22.0f),
        make_sample(3, 23.0f), make_sample(4, 24.0f)
    };
    memory_unit_process(&ring, batch, 5);

    summary_record_t rec;
    uint32_t n = memory_unit_drain(&ring, &rec, 1);
    CHECK(n == 1, "one summary record produced");
    CHECK(rec.samples_in == 5, "samples_in reflects full batch");
    CHECK(rec.samples_kept == 5, "no samples rejected when all are close together");
    CHECK(approx(rec.mean_c, 22.0f, 0.001f), "mean computed correctly");
    CHECK(approx(rec.min_c, 20.0f, 0.001f), "min computed correctly");
    CHECK(approx(rec.max_c, 24.0f, 0.001f), "max computed correctly");
    CHECK(rec.stddev_c > 0.0f, "stddev is non-negative and non-zero for varying data");

    int all_flagged_kept = 1;
    for (int i = 0; i < 5; i++) if (batch[i].is_outlier) all_flagged_kept = 0;
    CHECK(all_flagged_kept, "no input sample flagged is_outlier");
}

static void test_outlier_rejection(void)
{
    printf("test_outlier_rejection\n");
    summary_ring_t ring;
    memory_unit_init(&ring);

    /* Cluster around 22C plus one obvious glitch far outside
     * OUTLIER_THRESHOLD_C of the rough mean. */
    raw_sample_t batch[6] = {
        make_sample(0, 22.0f), make_sample(1, 22.1f), make_sample(2, 21.9f),
        make_sample(3, 22.2f), make_sample(4, 21.8f), make_sample(5, 60.0f) /* glitch */
    };
    memory_unit_process(&ring, batch, 6);

    summary_record_t rec;
    memory_unit_drain(&ring, &rec, 1);

    CHECK(batch[5].is_outlier == 1, "glitch sample flagged as outlier");
    CHECK(rec.samples_kept == 5, "exactly one sample rejected from a batch of 6");
    CHECK(rec.max_c < 25.0f, "max_c excludes the glitch value");
    CHECK(approx(rec.mean_c, 22.0f, 0.5f), "mean stays close to cluster center despite glitch");
}

static void test_degenerate_all_outliers_no_crash(void)
{
    printf("test_degenerate_all_outliers_no_crash\n");
    summary_ring_t ring;
    memory_unit_init(&ring);

    /* Every value is more than OUTLIER_THRESHOLD_C from the rough
     * mean of the batch (alternating extremes) so the "kept" count
     * degenerates to zero; this exercises the divide-by-zero guard. */
    raw_sample_t batch[2] = {
        make_sample(0, -50.0f), make_sample(1, 50.0f)
    };
    memory_unit_process(&ring, batch, 2);

    summary_record_t rec;
    uint32_t n = memory_unit_drain(&ring, &rec, 1);
    CHECK(n == 1, "a summary record is still produced for a degenerate batch");
    CHECK(rec.samples_kept == 0, "both samples rejected in degenerate case");
    CHECK(rec.stddev_c == 0.0f, "stddev falls back to 0 instead of NaN/garbage");
}

static void test_zero_length_batch_no_crash(void)
{
    printf("test_zero_length_batch_no_crash\n");
    summary_ring_t ring;
    memory_unit_init(&ring);

    memory_unit_process(&ring, NULL, 0);
    CHECK(ring.count == 0, "zero-length batch produces no summary record");
}

static void test_ring_wraps_after_capacity(void)
{
    printf("test_ring_wraps_after_capacity\n");
    summary_ring_t ring;
    memory_unit_init(&ring);

    raw_sample_t batch[1];
    for (uint32_t i = 0; i < SUMMARY_RING_SIZE + 5u; i++) {
        batch[0] = make_sample(i, 22.0f + (float)i);
        memory_unit_process(&ring, batch, 1);
    }
    CHECK(ring.count == SUMMARY_RING_SIZE, "summary ring count caps at SUMMARY_RING_SIZE");
}

int main(void)
{
    printf("=== memory_unit test suite ===\n");
    test_init_resets_state();
    test_basic_stats_no_outliers();
    test_outlier_rejection();
    test_degenerate_all_outliers_no_crash();
    test_zero_length_batch_no_crash();
    test_ring_wraps_after_capacity();

    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
