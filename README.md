# agent-rhythm-c

C99 port of the [agent-rhythm](https://github.com/SuperInstance/agent-rhythm) Python library — autocorrelation-based rhythm detection, pattern matching, and beat tracking.

Designed for embedded audio/rhythm detection on microcontrollers.

## What It Does

- **Rhythm representation** — beats, time signatures, inter-onset intervals
- **Pattern matching** — match rhythms against a built-in library (four-on-floor, tresillo, son clave, rumba clave, shuffle, bo-diddley) using circular cross-correlation
- **Syncopation detection** — score how off-beat a rhythm is
- **Polyrhythm detection** — identify competing rhythmic layers
- **Autocorrelation** — estimate periodicity of inter-onset intervals
- **Onset detection** — find note onsets from an amplitude envelope using spectral flux

## C API

```c
#include <agent_rhythm.h>

/* Create a steady pulse */
ArRhythm r;
ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);

/* Match against known patterns */
ArPatternMatcher pm;
ar_pattern_matcher_init(&pm);
const ArPattern *p = ar_pattern_best_match(&pm, &r);
printf("Match: %s\n", p->name);

/* Detect syncopation */
double sync = ar_detect_syncopation(&r);

/* Estimate period */
int period = ar_estimate_period(&r);
```

## Build

```sh
make        # builds libagent_rhythm.a
make test   # builds and runs tests
make clean
```

No dependencies beyond C99, `libc`, and `libm`.

## Port Notes

Ported from Python with these changes:
- Fixed-size arrays (stack-allocated) instead of dynamic lists, suitable for embedded
- `ArRhythm` holds up to 4096 beats, grids up to 256 steps
- Pattern matching uses integer grids and circular correlation
- Autocorrelation uses direct computation (no FFT dependency)
- Onset detection uses spectral flux with configurable threshold

## License

MIT
