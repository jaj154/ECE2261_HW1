#include <stdio.h>
#include "io_unit.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("  FAIL: %s\n", msg); failures++; } \
    else         { printf("  PASS: %s\n", msg); } \
} while (0)

static void test_init_resets_state(void)
{
    printf("test_init_resets_state\n");
    raw_ring_t ring;
    ring.head = 5; ring.count = 5; /* start from dirty state */
    io_unit_init(&ring);
    CHECK(ring.head == 0, "head reset to 0 after init");
    CHECK(ring.count == 0, "count reset to 0 after init");
}

static void test_sample_and_drain_fifo_order(void)
{
    printf("test_sample_and_drain_fifo_order\n");
    raw_ring_t ring;
    io_unit_init(&ring);

    for (uint32_t t = 0; t < 5; t++) {
        io_unit_sample(&ring, t);
    }
    CHECK(ring.count == 5, "count reflects 5 samples taken");

    raw_sample_t out[5];
    uint32_t n = io_unit_drain(&ring, out, 5);
    CHECK(n == 5, "drain returns all 5 samples");

    int order_ok = 1;
    for (uint32_t i = 0; i < 5; i++) {
        if (out[i].tick != i) order_ok = 0;
    }
    CHECK(order_ok, "drained samples preserve FIFO tick order");
    CHECK(ring.count == 0, "ring empty after full drain");
}

static void test_ring_overwrites_oldest_on_overflow(void)
{
    printf("test_ring_overwrites_oldest_on_overflow\n");
    raw_ring_t ring;
    io_unit_init(&ring);

    uint32_t total = RAW_RING_SIZE + 10u; /* force wraparound */
    for (uint32_t t = 0; t < total; t++) {
        io_unit_sample(&ring, t);
    }
    CHECK(ring.count == RAW_RING_SIZE, "count caps at RAW_RING_SIZE, does not overflow the struct");

    raw_sample_t out[RAW_RING_SIZE];
    uint32_t n = io_unit_drain(&ring, out, RAW_RING_SIZE);
    CHECK(n == RAW_RING_SIZE, "drain returns exactly RAW_RING_SIZE samples");
    CHECK(out[0].tick == (total - RAW_RING_SIZE), "oldest surviving sample has the correct post-overwrite tick");
    CHECK(out[RAW_RING_SIZE - 1].tick == total - 1, "newest sample matches the last tick sampled");
}

static void test_partial_drain_leaves_remainder(void)
{
    printf("test_partial_drain_leaves_remainder\n");
    raw_ring_t ring;
    io_unit_init(&ring);
    for (uint32_t t = 0; t < 10; t++) io_unit_sample(&ring, t);

    raw_sample_t out[4];
    uint32_t n = io_unit_drain(&ring, out, 4);
    CHECK(n == 4, "partial drain returns exactly the requested count");
    CHECK(ring.count == 6, "remaining count reflects undrained samples");
}

int main(void)
{
    printf("=== io_unit test suite ===\n");
    test_init_resets_state();
    test_sample_and_drain_fifo_order();
    test_ring_overwrites_oldest_on_overflow();
    test_partial_drain_leaves_remainder();

    printf("\n%d failure(s)\n", failures);
    return failures == 0 ? 0 : 1;
}
