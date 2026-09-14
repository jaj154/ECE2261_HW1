#ifndef INSTRUMENTATION_H
#define INSTRUMENTATION_H

#include <stdint.h>

typedef struct {
    const char *name;
    uint64_t    call_count;
    double      total_ns;
    double      min_ns;
    double      max_ns;
} stage_stats_t;

void   stage_stats_init(stage_stats_t *s, const char *name);
void   stage_stats_record(stage_stats_t *s, double elapsed_ns);
void   stage_stats_report(const stage_stats_t *s, double wall_seconds);
double now_ns(void);

#endif /* INSTRUMENTATION_H */
