#define _POSIX_C_SOURCE 199309L

#include "instrumentation.h"
#include <stdio.h>
#include <float.h>
#include <time.h>

double now_ns(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec * 1e9 + (double)ts.tv_nsec;
}

void stage_stats_init(stage_stats_t *s, const char *name)
{
    s->name = name;
    s->call_count = 0;
    s->total_ns = 0.0;
    s->min_ns = DBL_MAX;
    s->max_ns = 0.0;
}

void stage_stats_record(stage_stats_t *s, double elapsed_ns)
{
    s->call_count++;
    s->total_ns += elapsed_ns;
    if (elapsed_ns < s->min_ns) s->min_ns = elapsed_ns;
    if (elapsed_ns > s->max_ns) s->max_ns = elapsed_ns;
}

void stage_stats_report(const stage_stats_t *s, double wall_seconds)
{
    double avg_us = (s->call_count ? (s->total_ns / (double)s->call_count) : 0.0) / 1000.0;
    double throughput = wall_seconds > 0.0 ? (double)s->call_count / wall_seconds : 0.0;
    double min_us = (s->call_count ? s->min_ns : 0.0) / 1000.0;

    printf("  %-14s calls=%-8llu avg=%8.3f us  min=%8.3f us  max=%9.3f us  rate=%9.2f calls/s\n",
           s->name,
           (unsigned long long)s->call_count,
           avg_us,
           min_us,
           s->max_ns / 1000.0,
           throughput);
}
