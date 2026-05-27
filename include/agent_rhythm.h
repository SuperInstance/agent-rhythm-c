#ifndef AGENT_RHYTHM_H
#define AGENT_RHYTHM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ─── Constants ────────────────────────────────────────────── */

#define AR_MAX_BEATS      4096
#define AR_MAX_GRID       256
#define AR_NUM_PATTERNS   6
#define AR_GRID_DENSITY   16  /* 16th-note grid resolution */

/* ─── Beat ─────────────────────────────────────────────────── */

typedef struct {
    double time;      /* absolute time in seconds */
    double velocity;  /* 0.0 – 1.0 */
    double duration;  /* seconds (0 = impulse) */
} ArBeat;

int  ar_beat_is_accent(const ArBeat *b);
int  ar_beat_is_rest(const ArBeat *b);

/* ─── TimeSignature ────────────────────────────────────────── */

typedef struct {
    int beats_per_bar;  /* default 4 */
    int beat_unit;      /* default 4 = quarter note */
} ArTimeSignature;

ArTimeSignature ar_time_signature_default(void);
int  ar_time_signature_is_compound(const ArTimeSignature *ts);
double ar_time_signature_bar_duration(const ArTimeSignature *ts, double bpm);

/* ─── Rhythm ───────────────────────────────────────────────── */

typedef struct {
    ArBeat beats[AR_MAX_BEATS];
    int    num_beats;
    double bpm;
    ArTimeSignature time_signature;
} ArRhythm;

void ar_rhythm_init(ArRhythm *r, double bpm, ArTimeSignature ts);

/* Constructors */
void ar_rhythm_from_intervals(ArRhythm *r, const double *intervals, int n,
                              double bpm, ArTimeSignature ts);
void ar_rhythm_from_grid(ArRhythm *r, const int *hits, int n,
                         double subdivision, double bpm, ArTimeSignature ts);
void ar_rhythm_steady(ArRhythm *r, int n_beats, double bpm,
                      ArTimeSignature ts, double velocity);

/* Analysis */
int    ar_rhythm_count(const ArRhythm *r);
double ar_rhythm_duration(const ArRhythm *r);
double ar_rhythm_density(const ArRhythm *r);
void   ar_rhythm_intervals(const ArRhythm *r, double *out, int *out_n);
int    ar_rhythm_bar_count(const ArRhythm *r);
void   ar_rhythm_quantize(ArRhythm *r, double grid);
void   ar_rhythm_stretch(ArRhythm *r, double factor);

/* ─── Pattern ──────────────────────────────────────────────── */

typedef struct {
    char name[32];
    char description[64];
    int  grid[AR_MAX_GRID];
    int  grid_len;
    double subdivision;
} ArPattern;

/* Built-in pattern library — access via ar_get_pattern(index) */
const ArPattern *ar_get_pattern(int index);

/* ─── Pattern matching ─────────────────────────────────────── */

typedef struct {
    const ArPattern *pattern;
    double score;
} ArMatchResult;

typedef struct {
    double match_threshold;  /* default 0.7 */
} ArPatternMatcher;

void ar_pattern_matcher_init(ArPatternMatcher *pm);

/* Match rhythm against known patterns. Returns up to max_results matches. */
int ar_pattern_match(const ArPatternMatcher *pm, const ArRhythm *r,
                     ArMatchResult *results, int max_results);

/* Get single best match */
const ArPattern *ar_pattern_best_match(const ArPatternMatcher *pm,
                                        const ArRhythm *r);

/* ─── Syncopation & polyrhythm detection ───────────────────── */

/* Score how syncopated a rhythm is (0 = none, 1 = maximum) */
double ar_detect_syncopation(const ArRhythm *r);

/* Detect polyrhythmic structure. Returns count of detected polyrhythms.
   Each result is (voice_a, voice_b, strength). */
typedef struct { int a, b; double strength; } ArPolyrhythm;
int ar_detect_polyrhythm(const ArRhythm *r, ArPolyrhythm *out, int max_out);

/* ─── Autocorrelation-based rhythm analysis ────────────────── */

/* Compute autocorrelation of inter-onset intervals.
   out_lags receives lag values, out_corr receives correlation values.
   Returns number of lags computed (max_lags or n-1, whichever is smaller). */
int ar_autocorrelation(const ArRhythm *r, int max_lags,
                       double *out_corr);

/* Estimate the dominant period (in beats) from autocorrelation.
   Returns the lag with the highest autocorrelation peak. */
int ar_estimate_period(const ArRhythm *r);

/* ─── Onset detection (from an amplitude envelope) ─────────── */

/* Detect onsets from an amplitude envelope using spectral flux.
   envelope: amplitude samples at sample_rate.
   onsets: output sample indices where onsets were detected.
   Returns number of onsets detected. */
int ar_detect_onsets(const double *envelope, int n_samples, double sample_rate,
                     double threshold, int *onsets, int max_onsets);

#ifdef __cplusplus
}
#endif

#endif /* AGENT_RHYTHM_H */
