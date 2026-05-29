# agent-rhythm-c

Rhythm analysis in pure C — beat/time signature handling, cadence detection via autocorrelation, tempo tracking, and rhythm construction from intervals, step grids, or steady pulses.

## What This Gives You

- **Beat and rhythm construction**: build rhythms from inter-onset intervals, step grids (hit/miss arrays), or steady pulses
- **Time signatures**: simple vs compound detection, bar duration calculation
- **Cadence analysis**: autocorrelation-based period detection with confidence and regularity scores
- **Tempo tracking**: median-based global BPM estimation from beat timestamps
- **Zero dependencies**: C99, only `math.h` and `stdlib.h`

## Quick Start

```c
#include "agent_rhythm.h"

/* Build a tresillo pattern from a step grid */
int hits[] = {1, 0, 0, 1, 0, 0, 1, 0};
Rhythm r = rhythm_from_grid(hits, 8, 0.25, 120.0);

printf("Duration: %.2f\n", rhythm_duration(&r));
printf("Density: %.2f\n", rhythm_density(&r));
printf("Bars: %d\n", rhythm_bar_count(&r));

/* Cadence detection on event timestamps */
double timestamps[] = {0.0, 0.5, 1.0, 1.5, 2.0, 2.5};
CadenceResult cadence = cadence_analyze(timestamps, 6, 1.0);
if (cadence.period > 0) {
    printf("Period: %.2f (confidence: %.2f)\n", cadence.period, cadence.confidence);
}

/* Global tempo from beat times */
double bpm = tempo_global_bpm(beat_times, n);
```

## API Reference

### Beat / Rhythm

| Function | Description |
|----------|-------------|
| `beat_new(time)` | Create a beat at `time` with default velocity |
| `beat_with_velocity(b, v)` | Set velocity (≥0.8 = accent) |
| `rhythm_from_intervals(intervals, n, bpm)` | Build rhythm from IOIs |
| `rhythm_from_grid(hits, n, subdivision, bpm)` | Build from hit/miss array |
| `rhythm_steady(n, bpm)` | N evenly-spaced beats |
| `rhythm_duration(r)` | Total duration in seconds |
| `rhythm_density(r)` | Fraction of active beats |
| `rhythm_bar_count(r)` | Number of complete bars |

### Time Signature

| Function | Description |
|----------|-------------|
| `time_signature_new(beats, unit)` | e.g. `(4, 4)` for 4/4 |
| `time_signature_is_compound(ts)` | True if compound (e.g. 6/8) |
| `time_signature_bar_duration(ts, bpm)` | Duration of one bar in seconds |

### Cadence Analysis

```c
typedef struct {
    double period;       /* -1 if not detected */
    double confidence;
    double regularity;
    double mean_interval;
    double std_interval;
    int burst_count;
} CadenceResult;

CadenceResult cadence_analyze(double *timestamps, int n, double burst_gap);
```

### Tempo

| Function | Description |
|----------|-------------|
| `tempo_global_bpm(beats, n)` | Median-based BPM from beat timestamps |

## How It Fits

C port of [agent-rhythm-rs](https://github.com/SuperInstance/agent-rhythm-rs). Part of the [SuperInstance OpenConstruct](https://github.com/SuperInstance/OpenConstruct) ecosystem. Use this when you need rhythm analysis in embedded audio, game engines, or any C-based environment.

## Testing

**21 assertions** covering rhythm construction, density, bar count, time signatures, cadence analysis, and tempo estimation.

## Installation

```bash
make test    # build and run tests
make         # build the library
make clean   # clean build artifacts
```

Requires a C99 compiler. No external dependencies.
