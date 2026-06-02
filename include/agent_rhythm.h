#ifndef AGENT_RHYTHM_H
#define AGENT_RHYTHM_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ── Beat ────────────────────────────────────────────────────────────── */

/** A single beat event with timing and dynamics */
typedef struct {
    double time;       /**< Onset time in seconds */
    double velocity;   /**< Velocity (0.0 to 1.0) */
    double duration;   /**< Duration in seconds (0.0 = unspecified) */
} Beat;

/**
 * Create a beat at a given time with default velocity (1.0) and no duration.
 */
Beat beat_new(double time);

/**
 * Set the velocity of a beat.
 * @param b  Beat to modify
 * @param v  New velocity (0.0 to 1.0)
 * @return   Modified beat
 */
Beat beat_with_velocity(Beat b, double v);

/**
 * Set the duration of a beat.
 * @param b  Beat to modify
 * @param d  Duration in seconds
 * @return   Modified beat
 */
Beat beat_with_duration(Beat b, double d);

/**
 * Check if a beat is an accent (velocity >= 0.8).
 */
int beat_is_accent(Beat *b);

/**
 * Compare two beats for equality.
 * @return 1 if equal, 0 otherwise
 */
int beat_eq(Beat *a, Beat *b);

/* ── Time signature ──────────────────────────────────────────────────── */

/** A musical time signature */
typedef struct {
    int beats_per_bar;  /**< Number of beats per bar */
    int beat_unit;      /**< Beat unit (4 = quarter note, 8 = eighth note) */
} TimeSignature;

/**
 * Create a time signature.
 * @param beats  Beats per bar (e.g., 4)
 * @param unit   Beat unit (e.g., 4 for quarter notes)
 */
TimeSignature time_signature_new(int beats, int unit);

/**
 * Check if a time signature is compound (beats_per_bar divisible by 3, > 3).
 * e.g., 6/8, 9/8, 12/8 are compound; 3/4 is simple triple, 4/4 is simple quadruple
 */
int time_signature_is_compound(TimeSignature *ts);

/**
 * Check if a time signature is simple (not compound).
 */
int time_signature_is_simple(TimeSignature *ts);

/**
 * Compute the duration of one bar in seconds.
 * @param ts   Time signature
 * @param bpm  Tempo in beats per minute
 * @return     Bar duration in seconds
 */
double time_signature_bar_duration(TimeSignature *ts, double bpm);

/* ── Rhythm ──────────────────────────────────────────────────────────── */

#define MAX_BEATS 1024

/** A rhythm pattern consisting of beat events */
typedef struct {
    Beat beats[MAX_BEATS];  /**< Beat events */
    int beat_count;         /**< Number of beats */
    double bpm;             /**< Tempo in beats per minute */
    TimeSignature time_signature; /**< Time signature */
} Rhythm;

/**
 * Create a rhythm from an array of inter-onset intervals.
 * @param intervals  Array of interval durations in seconds
 * @param n          Number of intervals
 * @param bpm        Tempo
 * @return           Rhythm with n+1 beats
 */
Rhythm rhythm_from_intervals(double *intervals, int n, double bpm);

/**
 * Create a rhythm from a binary hit grid.
 * @param hits         Array of 0/1 values
 * @param n            Length of grid
 * @param subdivision  Duration of each grid cell in seconds
 * @param bpm          Tempo
 * @return             Rhythm with only the hit positions
 */
Rhythm rhythm_from_grid(int *hits, int n, double subdivision, double bpm);

/**
 * Create a steady (isochronous) rhythm.
 * @param n    Number of beats
 * @param bpm  Tempo
 * @return     Steady rhythm
 */
Rhythm rhythm_steady(int n, double bpm);

/**
 * Compute the total duration of a rhythm (last beat - first beat).
 */
double rhythm_duration(Rhythm *r);

/**
 * Compute the density of a rhythm (beats per second).
 */
double rhythm_density(Rhythm *r);

/**
 * Compute the number of complete bars in a rhythm.
 */
int rhythm_bar_count(Rhythm *r);

/**
 * Get the inter-onset intervals of a rhythm.
 * @param r      Rhythm
 * @param out    Output array (must hold at least beat_count-1 doubles)
 * @param max_n  Maximum number of intervals to write
 * @return       Number of intervals written
 */
int rhythm_intervals(Rhythm *r, double *out, int max_n);

/**
 * Compute the mean inter-onset interval.
 */
double rhythm_mean_interval(Rhythm *r);

/**
 * Compute the standard deviation of inter-onset intervals.
 */
double rhythm_std_interval(Rhythm *r);

/**
 * Check if a rhythm is empty (no beats).
 */
static inline int rhythm_is_empty(Rhythm *r) {
    return !r || r->beat_count == 0;
}

/* ── Cadence analysis ────────────────────────────────────────────────── */

/** Result of analyzing cadence from a sequence of timestamps */
typedef struct {
    double period;          /**< Detected period (seconds), -1 if not detected */
    double confidence;      /**< Detection confidence (0-1) */
    double regularity;      /**< Regularity metric (0-1, based on coefficient of variation) */
    double mean_interval;   /**< Mean inter-onset interval */
    double std_interval;    /**< Std dev of inter-onset intervals */
    int burst_count;        /**< Number of detected bursts */
} CadenceResult;

/**
 * Analyze the cadence of a sequence of timestamps.
 * @param timestamps  Array of event timestamps (seconds, must be sorted)
 * @param n           Number of timestamps (minimum 4 for period detection)
 * @param burst_gap   Gap threshold for burst detection (seconds)
 * @return            Cadence analysis result
 */
CadenceResult cadence_analyze(double *timestamps, int n, double burst_gap);

/* ── Tempo tracking ──────────────────────────────────────────────────── */

/**
 * Estimate the global tempo from beat timestamps.
 * Uses the median interval for robustness.
 * @param beat_times  Array of beat timestamps (seconds)
 * @param n           Number of timestamps
 * @return            Estimated BPM (0.0 if n < 2)
 */
double tempo_global_bpm(double *beat_times, int n);

/* ── Swing ───────────────────────────────────────────────────────────── */

/**
 * Apply swing to a rhythm by delaying every other beat.
 * @param r            Rhythm to modify
 * @param swing_ratio  Swing factor (0.0 = straight, 1.0 = full triplet swing)
 * @return             Modified rhythm
 */
Rhythm rhythm_swing(Rhythm *r, double swing_ratio);

/* ── Benchmark ───────────────────────────────────────────────────────── */

/**
 * Run a cadence analysis benchmark.
 * @param iterations  Number of iterations
 * @return            Elapsed time in seconds
 */
double benchmark_cadence(int iterations);

#endif
