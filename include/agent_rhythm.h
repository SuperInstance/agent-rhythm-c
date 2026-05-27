#ifndef AGENT_RHYTHM_H
#define AGENT_RHYTHM_H

#include <math.h>
#include <stdlib.h>
#include <string.h>

/* ── Beat ────────────────────────────────────────────────────────────── */

typedef struct {
    double time;
    double velocity;
    double duration;
} Beat;

Beat beat_new(double time);
Beat beat_with_velocity(Beat b, double v);
int beat_is_accent(Beat *b);

/* ── Time signature ──────────────────────────────────────────────────── */

typedef struct {
    int beats_per_bar;
    int beat_unit;
} TimeSignature;

TimeSignature time_signature_new(int beats, int unit);
int time_signature_is_compound(TimeSignature *ts);
double time_signature_bar_duration(TimeSignature *ts, double bpm);

/* ── Rhythm ──────────────────────────────────────────────────────────── */

#define MAX_BEATS 1024

typedef struct {
    Beat beats[MAX_BEATS];
    int beat_count;
    double bpm;
    TimeSignature time_signature;
} Rhythm;

Rhythm rhythm_from_intervals(double *intervals, int n, double bpm);
Rhythm rhythm_from_grid(int *hits, int n, double subdivision, double bpm);
Rhythm rhythm_steady(int n, double bpm);
double rhythm_duration(Rhythm *r);
double rhythm_density(Rhythm *r);
int rhythm_bar_count(Rhythm *r);

/* ── Cadence analysis ────────────────────────────────────────────────── */

typedef struct {
    double period;          /* -1 if not detected */
    double confidence;
    double regularity;
    double mean_interval;
    double std_interval;
    int burst_count;
} CadenceResult;

CadenceResult cadence_analyze(double *timestamps, int n, double burst_gap);

/* ── Tempo tracking ──────────────────────────────────────────────────── */

double tempo_global_bpm(double *beat_times, int n);

#endif
