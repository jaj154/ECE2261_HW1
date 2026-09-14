#include <stdio.h>
#include <stdint.h>

#include "pipeline_types.h"
#include "io_unit.h"
#include "memory_unit.h"
#include "compute_unit.h"
#include "instrumentation.h"

#define TOTAL_TICKS   4096u
#define MEMORY_WINDOW 16u /* raw samples ingested per summarization batch */

static const char *hvac_state_str(hvac_state_t s)
{
    switch (s) {
        case HVAC_STATE_NOMINAL:       return "NOMINAL";
        case HVAC_STATE_UNDERCOOLING:  return "UNDERCOOLING (rising trend)";
        case HVAC_STATE_OVERCOOLING:   return "OVERCOOLING (falling trend)";
        case HVAC_STATE_SHORT_CYCLING: return "SHORT-CYCLING (oscillation)";
        default:                       return "UNKNOWN";
    }
}

int main(void)
{
    /* Bare-metal analog: these would be statically allocated globals
     * or .bss-resident buffers - no heap on the target, so none here
     * either. */
    raw_ring_t     raw_ring;
    summary_ring_t summary_ring;

    raw_sample_t     raw_batch[MEMORY_WINDOW];
    summary_record_t predict_batch[PREDICT_WINDOW];

    stage_stats_t io_stats, mem_stats, compute_stats;
    stage_stats_init(&io_stats, "io_unit");
    stage_stats_init(&mem_stats, "memory_unit");
    stage_stats_init(&compute_stats, "compute_unit");

    io_unit_init(&raw_ring);
    memory_unit_init(&summary_ring);

    uint32_t summaries_produced = 0;
    uint32_t predictions_made   = 0;
    uint64_t total_samples_in   = 0;
    uint64_t total_samples_kept = 0;

    printf("=== Critter Pipeline Simulation (bare-metal desktop stand-in) ===\n");
    printf("Ticks: %u | Memory window: %u samples | Predict window: %u summaries\n\n",
           TOTAL_TICKS, MEMORY_WINDOW, PREDICT_WINDOW);

    double t_start = now_ns();

    for (uint32_t tick = 0; tick < TOTAL_TICKS; tick++) {

        /* --- I/O unit: high-rate sampling, runs every tick ------------ */
        double t0 = now_ns();
        io_unit_sample(&raw_ring, tick);
        stage_stats_record(&io_stats, now_ns() - t0);

        /* --- Memory unit: fires once a full batch has accumulated ----- */
        if ((tick + 1u) % MEMORY_WINDOW == 0u) {
            uint32_t drained = io_unit_drain(&raw_ring, raw_batch, MEMORY_WINDOW);

            double t1 = now_ns();
            memory_unit_process(&summary_ring, raw_batch, drained);
            stage_stats_record(&mem_stats, now_ns() - t1);

            summaries_produced++;
            total_samples_in += drained;
            for (uint32_t i = 0; i < drained; i++) {
                if (!raw_batch[i].is_outlier) total_samples_kept++;
            }

            /* --- Compute unit: fires once enough summaries exist ----- */
            if (summaries_produced % PREDICT_WINDOW == 0u) {
                uint32_t sdrained = memory_unit_drain(&summary_ring, predict_batch, PREDICT_WINDOW);

                prediction_t pred;
                double t2 = now_ns();
                compute_unit_predict(predict_batch, sdrained, &pred);
                stage_stats_record(&compute_stats, now_ns() - t2);

                predictions_made++;
                printf("[tick %5u] trend=%+7.4f C/tick  projected=%6.2fC  state=%s\n",
                       pred.tick, pred.trend_slope_c_per_tick,
                       pred.projected_temp_c, hvac_state_str(pred.state));
            }
        }
    }

    double t_end = now_ns();
    double wall_seconds = (t_end - t_start) / 1e9;

    printf("\n=== Pipeline Stage Timing (wall clock: %.4f s) ===\n", wall_seconds);
    stage_stats_report(&io_stats, wall_seconds);
    stage_stats_report(&mem_stats, wall_seconds);
    stage_stats_report(&compute_stats, wall_seconds);

    printf("\n=== Memory Footprint (why the memory unit exists) ===\n");
    size_t raw_bytes  = (size_t)TOTAL_TICKS * sizeof(raw_sample_t);
    size_t summ_bytes = (size_t)summaries_produced * sizeof(summary_record_t);
    printf("  raw samples ingested   : %u  (%zu bytes if all retained)\n", TOTAL_TICKS, raw_bytes);
    printf("  summary records stored : %u  (%zu bytes)\n", summaries_produced, summ_bytes);
    if (summ_bytes > 0) {
        printf("  compression ratio      : %.1fx\n", (double)raw_bytes / (double)summ_bytes);
    }

    printf("\n=== Outlier Rejection (memory unit) ===\n");
    uint64_t rejected = total_samples_in - total_samples_kept;
    double reject_pct = total_samples_in ? 100.0 * (double)rejected / (double)total_samples_in : 0.0;
    printf("  samples in   : %llu\n", (unsigned long long)total_samples_in);
    printf("  samples kept : %llu\n", (unsigned long long)total_samples_kept);
    printf("  rejected     : %llu (%.2f%%)\n", (unsigned long long)rejected, reject_pct);

    printf("\n=== Predictions ===\n");
    printf("  predictions produced : %u\n", predictions_made);

    return 0;
}
