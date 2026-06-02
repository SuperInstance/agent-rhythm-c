#define _POSIX_C_SOURCE 199309L
#include "agent_rhythm.h"
#include <float.h>
#include <time.h>

/* ── Beat ────────────────────────────────────────────────────────────── */

Beat beat_new(double time) {
    Beat b;
    b.time = time;
    b.velocity = 1.0;
    b.duration = 0.0;
    return b;
}

Beat beat_with_velocity(Beat b, double v) {
    b.velocity = v;
    return b;
}

Beat beat_with_duration(Beat b, double d) {
    b.duration = d;
    return b;
}

int beat_is_accent(Beat *b) {
    if (!b) return 0;
    return b->velocity >= 0.8;
}

int beat_eq(Beat *a, Beat *b) {
    if (!a || !b) return 0;
    return fabs(a->time - b->time) < 1e-10 &&
           fabs(a->velocity - b->velocity) < 1e-10 &&
           fabs(a->duration - b->duration) < 1e-10;
}

/* ── Time signature ──────────────────────────────────────────────────── */

TimeSignature time_signature_new(int beats, int unit) {
    TimeSignature ts;
    ts.beats_per_bar = beats;
    ts.beat_unit = unit;
    return ts;
}

int time_signature_is_compound(TimeSignature *ts) {
    if (!ts) return 0;
    return ts->beats_per_bar % 3 == 0 && ts->beats_per_bar > 3;
}

int time_signature_is_simple(TimeSignature *ts) {
    if (!ts) return 0;
    return !time_signature_is_compound(ts);
}

double time_signature_bar_duration(TimeSignature *ts, double bpm) {
    if (!ts || bpm <= 0.0) return 0.0;
    return ts->beats_per_bar / (bpm / 60.0);
}

/* ── Rhythm ──────────────────────────────────────────────────────────── */

Rhythm rhythm_from_intervals(double *intervals, int n, double bpm) {
    Rhythm r;
    r.bpm = bpm;
    r.time_signature = time_signature_new(4, 4);
    r.beat_count = 0;
    if (!intervals) return r;
    double t = 0.0;
    for (int i = 0; i < n && r.beat_count < MAX_BEATS; i++) {
        r.beats[r.beat_count++] = beat_new(t);
        t += intervals[i];
    }
    return r;
}

Rhythm rhythm_from_grid(int *hits, int n, double subdivision, double bpm) {
    Rhythm r;
    r.bpm = bpm;
    r.time_signature = time_signature_new(4, 4);
    r.beat_count = 0;
    if (!hits) return r;
    double t = 0.0;
    for (int i = 0; i < n && r.beat_count < MAX_BEATS; i++) {
        if (hits[i]) {
            r.beats[r.beat_count++] = beat_new(t);
        }
        t += subdivision;
    }
    return r;
}

Rhythm rhythm_steady(int n, double bpm) {
    Rhythm r;
    r.bpm = bpm;
    r.time_signature = time_signature_new(4, 4);
    r.beat_count = 0;
    if (n <= 0 || bpm <= 0.0) return r;
    double interval = 60.0 / bpm;
    for (int i = 0; i < n && r.beat_count < MAX_BEATS; i++) {
        r.beats[r.beat_count++] = beat_new(i * interval);
    }
    return r;
}

double rhythm_duration(Rhythm *r) {
    if (!r || r->beat_count < 2) return 0.0;
    return r->beats[r->beat_count - 1].time - r->beats[0].time;
}

double rhythm_density(Rhythm *r) {
    double d = rhythm_duration(r);
    if (d <= 0.0) return 0.0;
    return (double)r->beat_count / d;
}

int rhythm_bar_count(Rhythm *r) {
    if (!r) return 0;
    double bar_dur = time_signature_bar_duration(&r->time_signature, r->bpm);
    if (bar_dur <= 0.0) return 0;
    return (int)(rhythm_duration(r) / bar_dur);
}

int rhythm_intervals(Rhythm *r, double *out, int max_n) {
    if (!r || !out || max_n <= 0) return 0;
    int count = r->beat_count - 1;
    if (count > max_n) count = max_n;
    for (int i = 0; i < count; i++) {
        out[i] = r->beats[i + 1].time - r->beats[i].time;
    }
    return count;
}

double rhythm_mean_interval(Rhythm *r) {
    if (!r || r->beat_count < 2) return 0.0;
    double sum = 0.0;
    int n = r->beat_count - 1;
    for (int i = 0; i < n; i++) {
        sum += r->beats[i + 1].time - r->beats[i].time;
    }
    return sum / n;
}

double rhythm_std_interval(Rhythm *r) {
    if (!r || r->beat_count < 2) return 0.0;
    double mean = rhythm_mean_interval(r);
    int n = r->beat_count - 1;
    double ss = 0.0;
    for (int i = 0; i < n; i++) {
        double d = (r->beats[i + 1].time - r->beats[i].time) - mean;
        ss += d * d;
    }
    return sqrt(ss / n);
}

/* ── Cadence analysis ────────────────────────────────────────────────── */

static double array_mean(double *data, int n) {
    if (n <= 0) return 0.0;
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += data[i];
    return sum / n;
}

static double array_std(double *data, int n) {
    if (n <= 0) return 0.0;
    double m = array_mean(data, n);
    double ss = 0.0;
    for (int i = 0; i < n; i++) ss += (data[i] - m) * (data[i] - m);
    return sqrt(ss / n);
}

CadenceResult cadence_analyze(double *timestamps, int n, double burst_gap) {
    CadenceResult cr;
    memset(&cr, 0, sizeof(cr));
    cr.period = -1;

    if (!timestamps || n < 4) return cr;

    double *intervals = (double *)malloc((size_t)(n - 1) * sizeof(double));
    for (int i = 0; i < n - 1; i++) intervals[i] = timestamps[i + 1] - timestamps[i];

    cr.mean_interval = array_mean(intervals, n - 1);
    cr.std_interval = array_std(intervals, n - 1);

    double cv = (cr.mean_interval > 0.0) ? cr.std_interval / cr.mean_interval : 1e9;
    cr.regularity = (1.0 - cv > 0.0) ? 1.0 - cv : 0.0;

    cr.burst_count = 1;
    for (int i = 1; i < n; i++) {
        if (timestamps[i] - timestamps[i - 1] > burst_gap) cr.burst_count++;
    }

    int max_lag = (n / 2);
    double best_corr = -1.0;
    int best_lag = 1;
    double im = array_mean(intervals, n - 1);

    for (int lag = 1; lag <= max_lag; lag++) {
        double numer = 0.0, denom_a = 0.0, denom_b = 0.0;
        for (int i = 0; i < n - 1 - lag; i++) {
            double da = intervals[i] - im;
            double db = intervals[i + lag] - im;
            numer += da * db;
            denom_a += da * da;
            denom_b += db * db;
        }
        double denom = sqrt(denom_a * denom_b);
        if (denom > 0.0) {
            double corr = numer / denom;
            if (corr > best_corr) {
                best_corr = corr;
                best_lag = lag;
            }
        }
    }

    if (best_corr > 0.15) {
        cr.period = 0.0;
        for (int i = 0; i < best_lag; i++) cr.period += intervals[i];
        cr.period /= best_lag;
        cr.confidence = (best_corr > 1.0) ? 1.0 : best_corr;
    }

    free(intervals);
    return cr;
}

/* ── Tempo tracking ──────────────────────────────────────────────────── */

double tempo_global_bpm(double *beat_times, int n) {
    if (!beat_times || n < 2) return 0.0;
    double *intervals = (double *)malloc((size_t)(n - 1) * sizeof(double));
    for (int i = 0; i < n - 1; i++) intervals[i] = beat_times[i + 1] - beat_times[i];

    for (int i = 0; i < n - 2; i++) {
        for (int j = i + 1; j < n - 1; j++) {
            if (intervals[j] < intervals[i]) {
                double tmp = intervals[i]; intervals[i] = intervals[j]; intervals[j] = tmp;
            }
        }
    }
    double median = intervals[(n - 1) / 2];
    free(intervals);
    return (median > 0.0) ? 60.0 / median : 0.0;
}

/* ── Swing ───────────────────────────────────────────────────────────── */

Rhythm rhythm_swing(Rhythm *r, double swing_ratio) {
    if (!r) return rhythm_steady(0, 120.0);
    Rhythm out = *r;
    double interval = 60.0 / r->bpm;
    double swing_delay = swing_ratio * interval * 0.5;
    for (int i = 1; i < out.beat_count; i += 2) {
        out.beats[i].time += swing_delay;
    }
    return out;
}

/* ── Benchmark ───────────────────────────────────────────────────────── */

double benchmark_cadence(int iterations) {
    double ts[20];
    for (int i = 0; i < 20; i++) ts[i] = i * 0.5;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    for (int i = 0; i < iterations; i++) {
        cadence_analyze(ts, 20, 5.0);
    }
    clock_gettime(CLOCK_MONOTONIC, &end);
    return (double)(end.tv_sec - start.tv_sec) +
           (double)(end.tv_nsec - start.tv_nsec) / 1e9;
}
