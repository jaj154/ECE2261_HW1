# Critter Pipeline Simulation

A single-process C simulation of the Critter's three-stage embedded pipeline,
written as a desktop stand-in for a bare-metal build (Raspberry Pi 5 / Geeny
IDE target). No heap allocation, no OS threads/RTOS, no third-party
libraries beyond the standard C library and libm — just a superloop calling
into three modules, the way a junior embedded developer would first put this
together before optimizing further (DMA, interrupts, RTOS tasks, etc.).

## Pipeline stages

1. **I/O unit** (`io_unit.c`, `sensor_hw.c`) — I/O-bound.
   Simulates a high-rate ADC poll every tick and pushes each raw sample into
   a small fixed-size ring buffer (`RAW_RING_SIZE = 64`). `sensor_hw.c` is
   the abstraction boundary: on real hardware this is where memory-mapped
   register access (`volatile uint32_t *`) would live; here it synthesizes a
   plausible machine-room temperature signal with drift, jitter, and
   occasional glitches.

2. **Memory unit** (`memory_unit.c`) — memory/data-focused.
   Every `MEMORY_WINDOW` (16) raw samples, it rejects outliers with a simple
   fixed-deviation-from-mean filter, then computes min/max/mean/stddev over
   the survivors and stores a single compact `summary_record_t`. This is the
   "optimize the data, remove outliers, summarize for offline analysis"
   role — trading a bit of CPU now for a large reduction in what has to be
   logged or transmitted later (see the compression ratio printed at the
   end of the run).

3. **Compute unit** (`compute_unit.c`) — compute-intensive.
   Every `PREDICT_WINDOW` (8) summary records, it fits a least-squares
   linear regression over the recent means to get a trend slope, checks for
   sign-change oscillation (short-cycling), and classifies the HVAC's
   apparent behavior as NOMINAL, UNDERCOOLING, OVERCOOLING, or
   SHORT-CYCLING, plus a naive forward projection of temperature.

`main.c` is the bare-metal superloop: it calls the I/O unit every tick, the
memory unit every `MEMORY_WINDOW` ticks, and the compute unit every
`PREDICT_WINDOW` summaries, all synchronously, with static buffers passed
between stages (no dynamic allocation anywhere in the pipeline).

## Instrumentation

`instrumentation.c` wraps each stage call with `clock_gettime(CLOCK_MONOTONIC)`
timing and reports, per stage: call count, average/min/max latency, and
effective call rate. This is meant to make the three stages' different
performance characteristics visible directly in the output — I/O unit calls
are frequent and each is cheap, the memory unit runs less often but does
more work per call, and the compute unit runs rarest but is the most
CPU-heavy per call.

At the end of the run the program also prints:
- the memory footprint comparison (raw samples vs. summary records) that
  motivates the memory unit's existence,
- outlier rejection totals,
- total predictions produced.

## Build & run

```sh
make
./critter_pipeline
```

## Adapting for real bare-metal (Raspberry Pi 5 / Geeny IDE)

To move this from simulation to the actual target:
- Replace `sensor_hw_read_raw()` with real ADC/temperature-peripheral
  register access (memory-mapped I/O via `volatile` pointers).
- Replace `now_ns()` / `clock_gettime()` with a read of the hardware cycle
  counter or a peripheral timer register, since there's no POSIX clock on
  bare metal.
- Move `io_unit_sample()` into a timer/SysTick interrupt handler instead of
  calling it once per superloop iteration, so sampling rate isn't tied to
  how fast the rest of the loop runs.
- Replace `printf()` diagnostics with UART output or a debug log buffer.
