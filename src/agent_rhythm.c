#include "agent_rhythm.h"
#include <float.h>

/* ── Beat ────────────────────────────────────────────────────────────── */

Beat beat_new(double time) {
    Beat b = { time, 1.0, 0.0 };
    return b;
}

Beat beat_with_velocity(Beat b, double v) {
    b.velocity = v;
    return b;
}

int beat_is_accent(Beat *b) {
    return b->velocity >= 0.8;
}

/* ── Time signature ──────────────────────────────────────────────────── */

TimeSignature time_signature_new(int beats, int unit) {
    TimeSignature ts = { beats, unit };
    return ts;
}

int time_signature_is_compound(TimeSignature *ts) {
    return ts->beats_per_bar % 3 == 0 && ts->beats_per_bar > 3;
}

double time_signature_bar_duration(TimeSignature *ts, double bpm) {
    if (bpm <= 0.0) return 0.0;
    return ts->beats_per_bar / (bpm / 60.0);
}

/* ── Rhythm ──────────────────────────────────────────────────────────── */

Rhythm rhythm_from_intervals(double *intervals, int n, double bpm) {
    Rhythm r;
    r.bpm = bpm;
    r.time_signature = time_signature_new(4, 4);
    r.beat_count = 0;
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
    double interval = 60.0 / bpm;
    for (int i = 0; i < n && r.beat_count < MAX_BEATS; i++) {
        r.beats[r.beat_count++] = beat_new(i * interval);
    }
    return r;
}

double rhythm_duration(Rhythm *r) {
    if (r->beat_count < 2) return 0.0;
    return r->beats[r->beat_count - 1].time - r->beats[0].time;
}

double rhythm_density(Rhythm *r) {
    double d = rhythm_duration(r);
    if (d <= 0.0) return 0.0;
    return (double)r->beat_count / d;
}

int rhythm_bar_count(Rhythm *r) {
    double bar_dur = time_signature_bar_duration(&r->time_signature, r->bpm);
    if (bar_dur <= 0.0) return 0;
    return (int)(rhythm_duration(r) / bar_dur);
}

/* ── Cadence analysis ────────────────────────────────────────────────── */

static double array_mean(double *data, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) sum += data[i];
    return sum / n;
}

static double array_std(double *data, int n) {
    double m = array_mean(data, n);
    double ss = 0.0;
    for (int i = 0; i < n; i++) ss += (data[i] - m) * (data[i] - m);
    return sqrt(ss / n);
}

CadenceResult cadence_analyze(double *timestamps, int n, double burst_gap) {
    CadenceResult cr = { -1, 0, 0, 0, 0, 0 };
    if (n < 4) return cr;

    /* Compute intervals */
    double *intervals = malloc((n - 1) * sizeof(double));
    for (int i = 0; i < n - 1; i++) intervals[i] = timestamps[i + 1] - timestamps[i];

    cr.mean_interval = array_mean(intervals, n - 1);
    cr.std_interval = array_std(intervals, n - 1);

    /* Regularity = 1 - CV */
    double cv = (cr.mean_interval > 0.0) ? cr.std_interval / cr.mean_interval : 1e9;
    cr.regularity = (1.0 - cv > 0.0) ? 1.0 - cv : 0.0;

    /* Burst count */
    cr.burst_count = 1;
    for (int i = 1; i < n; i++) {
        if (timestamps[i] - timestamps[i - 1] > burst_gap) cr.burst_count++;
    }

    /* Period detection via simple autocorrelation */
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
    if (n < 2) return 0.0;
    double *intervals = malloc((n - 1) * sizeof(double));
    for (int i = 0; i < n - 1; i++) intervals[i] = beat_times[i + 1] - beat_times[i];

    /* Sort for median */
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
