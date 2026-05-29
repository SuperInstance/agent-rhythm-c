# agent-rhythm-c

C port of [agent-rhythm](https://github.com/SuperInstance/agent-rhythm) — rhythm analysis for music and behavioral patterns.

## Features

- **Beat/Note representation**: time, velocity, duration
- **Rhythm construction**: from intervals, step grids, or steady pulses
- **Time signatures**: simple/compound detection, bar duration
- **Cadence analysis**: autocorrelation-based period detection
- **Tempo tracking**: median-based global BPM estimation
- **Zero dependencies**: C99, only `math.h` and `stdlib.h`

## Build & Test

```bash
make test
```

## API

```c
#include "agent_rhythm.h"

/* Tresillo pattern */
int hits[] = {1, 0, 0, 1, 0, 0, 1, 0};
Rhythm r = rhythm_from_grid(hits, 8, 0.25, 120.0);

double density = rhythm_density(&r);
int bars = rhythm_bar_count(&r);

/* Tempo from timestamps */
double bpm = tempo_global_bpm(beat_times, n);
```

## License

MIT

Part of the [SuperInstance OpenConstruct](https://github.com/SuperInstance/OpenConstruct) ecosystem.
