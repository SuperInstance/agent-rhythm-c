/*
 * agent_rhythm — rhythm detection, pattern matching, beat tracking
 *
 * C99 port of the Python agent-rhythm library.
 * For embedded audio/rhythm detection on microcontrollers.
 */

#include "agent_rhythm.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

/* ─── Beat helpers ─────────────────────────────────────────── */

int ar_beat_is_accent(const ArBeat *b) { return b->velocity >= 0.8; }
int ar_beat_is_rest(const ArBeat *b)   { return b->velocity == 0.0; }

/* ─── TimeSignature ────────────────────────────────────────── */

ArTimeSignature ar_time_signature_default(void)
{
    ArTimeSignature ts = {4, 4};
    return ts;
}

int ar_time_signature_is_compound(const ArTimeSignature *ts)
{
    return ts->beats_per_bar % 3 == 0 && ts->beats_per_bar > 3;
}

double ar_time_signature_bar_duration(const ArTimeSignature *ts, double bpm)
{
    return (double)ts->beats_per_bar / (bpm / 60.0);
}

/* ─── Rhythm constructors ──────────────────────────────────── */

void ar_rhythm_init(ArRhythm *r, double bpm, ArTimeSignature ts)
{
    memset(r, 0, sizeof(*r));
    r->bpm = bpm;
    r->time_signature = ts;
}

void ar_rhythm_from_intervals(ArRhythm *r, const double *intervals, int n,
                              double bpm, ArTimeSignature ts)
{
    ar_rhythm_init(r, bpm, ts);
    double t = 0.0;
    for (int i = 0; i < n && r->num_beats < AR_MAX_BEATS; i++) {
        r->beats[r->num_beats].time = t;
        r->beats[r->num_beats].velocity = 1.0;
        r->beats[r->num_beats].duration = 0.0;
        r->num_beats++;
        t += intervals[i];
    }
}

void ar_rhythm_from_grid(ArRhythm *r, const int *hits, int n,
                         double subdivision, double bpm, ArTimeSignature ts)
{
    ar_rhythm_init(r, bpm, ts);
    double t = 0.0;
    for (int i = 0; i < n && r->num_beats < AR_MAX_BEATS; i++) {
        if (hits[i]) {
            r->beats[r->num_beats].time = t;
            r->beats[r->num_beats].velocity = 1.0;
            r->beats[r->num_beats].duration = 0.0;
            r->num_beats++;
        }
        t += subdivision;
    }
}

void ar_rhythm_steady(ArRhythm *r, int n_beats, double bpm,
                      ArTimeSignature ts, double velocity)
{
    ar_rhythm_init(r, bpm, ts);
    double interval = 60.0 / bpm;
    for (int i = 0; i < n_beats && r->num_beats < AR_MAX_BEATS; i++) {
        r->beats[i].time = i * interval;
        r->beats[i].velocity = velocity;
        r->beats[i].duration = 0.0;
    }
    r->num_beats = (n_beats < AR_MAX_BEATS) ? n_beats : AR_MAX_BEATS;
}

/* ─── Rhythm analysis ──────────────────────────────────────── */

int ar_rhythm_count(const ArRhythm *r) { return r->num_beats; }

double ar_rhythm_duration(const ArRhythm *r)
{
    if (r->num_beats < 2) return 0.0;
    return r->beats[r->num_beats - 1].time - r->beats[0].time;
}

double ar_rhythm_density(const ArRhythm *r)
{
    double d = ar_rhythm_duration(r);
    return (d > 0) ? (double)r->num_beats / d : 0.0;
}

void ar_rhythm_intervals(const ArRhythm *r, double *out, int *out_n)
{
    *out_n = 0;
    for (int i = 1; i < r->num_beats; i++) {
        out[*out_n] = r->beats[i].time - r->beats[i - 1].time;
        (*out_n)++;
    }
}

int ar_rhythm_bar_count(const ArRhythm *r)
{
    double bar_dur = ar_time_signature_bar_duration(&r->time_signature, r->bpm);
    if (bar_dur <= 0) return 0;
    return (int)(ar_rhythm_duration(r) / bar_dur);
}

void ar_rhythm_quantize(ArRhythm *r, double grid)
{
    for (int i = 0; i < r->num_beats; i++) {
        r->beats[i].time = round(r->beats[i].time / grid) * grid;
    }
}

void ar_rhythm_stretch(ArRhythm *r, double factor)
{
    for (int i = 0; i < r->num_beats; i++) {
        r->beats[i].time *= factor;
        r->beats[i].duration *= factor;
    }
    r->bpm /= factor;
}

/* ─── Built-in patterns ────────────────────────────────────── */

static const int grid_four_on_floor[] = {1,0,0,0, 1,0,0,0, 1,0,0,0, 1,0,0,0};
static const int grid_bo_diddley[] = {1,1,0,1,1,1,0,1,1,0,1,0};
static const int grid_tresillo[] = {1,0,0,1,0,0,1,0};
static const int grid_son_clave[] = {1,0,1,0,0,1,1,0,1,0,0,0,1,0,0,0};
static const int grid_rumba_clave[] = {1,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0};
static const int grid_shuffle[] = {1,0,1,0,1,0,1,0,1,0,1,0};

#define MAKE_PATTERN(idx, name_str, desc, g, sub) \
    { name_str, desc, {0}, (int)(sizeof(g)/sizeof(g[0])), sub }

/* Pattern data stored in patterns_storage, initialized at runtime */

/* Mutable storage for built-in patterns */
static ArPattern patterns_storage[AR_NUM_PATTERNS];
static int patterns_initialized = 0;

static void init_patterns(void)
{
    if (patterns_initialized) return;

    strcpy(patterns_storage[0].name, "four-on-floor");
    strcpy(patterns_storage[0].description, "Classic kick — every quarter note");
    patterns_storage[0].grid_len = 16;
    patterns_storage[0].subdivision = 0.25;
    memcpy(patterns_storage[0].grid, grid_four_on_floor, sizeof(grid_four_on_floor));

    strcpy(patterns_storage[1].name, "bo-diddley");
    strcpy(patterns_storage[1].description, "3-3-4-2 hambone rhythm");
    patterns_storage[1].grid_len = 12;
    patterns_storage[1].subdivision = 1.0/6.0;
    memcpy(patterns_storage[1].grid, grid_bo_diddley, sizeof(grid_bo_diddley));

    strcpy(patterns_storage[2].name, "tresillo");
    strcpy(patterns_storage[2].description, "Cuban 3+3+2 over 8 steps");
    patterns_storage[2].grid_len = 8;
    patterns_storage[2].subdivision = 0.25;
    memcpy(patterns_storage[2].grid, grid_tresillo, sizeof(grid_tresillo));

    strcpy(patterns_storage[3].name, "son-clave");
    strcpy(patterns_storage[3].description, "2-3 son clave");
    patterns_storage[3].grid_len = 16;
    patterns_storage[3].subdivision = 0.125;
    memcpy(patterns_storage[3].grid, grid_son_clave, sizeof(grid_son_clave));

    strcpy(patterns_storage[4].name, "rumba-clave");
    strcpy(patterns_storage[4].description, "2-3 rumba clave");
    patterns_storage[4].grid_len = 16;
    patterns_storage[4].subdivision = 0.125;
    memcpy(patterns_storage[4].grid, grid_rumba_clave, sizeof(grid_rumba_clave));

    strcpy(patterns_storage[5].name, "shuffle");
    strcpy(patterns_storage[5].description, "Swung 8th-note feel");
    patterns_storage[5].grid_len = 12;
    patterns_storage[5].subdivision = 1.0/6.0;
    memcpy(patterns_storage[5].grid, grid_shuffle, sizeof(grid_shuffle));

    patterns_initialized = 1;
}

const ArPattern *ar_get_pattern(int index)
{
    init_patterns();
    return (index >= 0 && index < AR_NUM_PATTERNS) ? &patterns_storage[index] : NULL;
}

/* ─── Grid conversion helpers ──────────────────────────────── */

static void rhythm_to_grid(const ArRhythm *r, double subdivision,
                           int *grid, int *grid_len)
{
    if (r->num_beats == 0) { *grid_len = 0; return; }
    double span = ar_rhythm_duration(r) + subdivision;
    int n = (int)(span / subdivision);
    if (n > AR_MAX_GRID) n = AR_MAX_GRID;
    memset(grid, 0, n * sizeof(int));
    for (int i = 0; i < r->num_beats; i++) {
        int idx = (int)round(r->beats[i].time / subdivision);
        if (idx >= 0 && idx < n) grid[idx] = 1;
    }
    *grid_len = n;
}

static void resample_grid(const int *src, int src_len, int *dst, int dst_len)
{
    for (int i = 0; i < dst_len; i++) {
        int src_idx = (int)((double)i * src_len / dst_len);
        if (src_idx < src_len)
            dst[i] = src[src_idx];
        else
            dst[i] = 0;
    }
}

static double circular_correlation(const int *a, int la, const int *b, int lb)
{
    int length = la > lb ? la : lb;
    int ap[AR_MAX_GRID], bp[AR_MAX_GRID];
    memset(ap, 0, sizeof(int) * (unsigned)length);
    memset(bp, 0, sizeof(int) * (unsigned)length);
    memcpy(ap, a, sizeof(int) * (unsigned)(la < length ? la : length));
    memcpy(bp, b, sizeof(int) * (unsigned)(lb < length ? lb : length));

    int total_on = 0;
    for (int i = 0; i < length; i++) total_on += ap[i] + bp[i];
    if (total_on == 0) return 0.0;

    int sum_a = 0, sum_b = 0;
    for (int i = 0; i < length; i++) { sum_a += ap[i]; sum_b += bp[i]; }
    int expected = sum_a < sum_b ? sum_a : sum_b;
    if (expected == 0) return 0.0;

    int best = 0;
    for (int shift = 0; shift < length; shift++) {
        int matches = 0;
        for (int i = 0; i < length; i++) {
            if (ap[i] && ap[i] == bp[(i + shift) % length])
                matches++;
        }
        if (matches > best) best = matches;
    }
    return (double)best / (double)expected;
}

/* ─── Pattern matching ─────────────────────────────────────── */

void ar_pattern_matcher_init(ArPatternMatcher *pm)
{
    pm->match_threshold = 0.7;
    init_patterns();
}

int ar_pattern_match(const ArPatternMatcher *pm, const ArRhythm *r,
                     ArMatchResult *results, int max_results)
{
    if (r->num_beats == 0) return 0;

    double interval = (r->bpm > 0) ? 60.0 / r->bpm : 0.5;
    double sub = interval / 4.0;

    int rhythm_grid[AR_MAX_GRID], rhythm_len;
    rhythm_to_grid(r, sub, rhythm_grid, &rhythm_len);
    if (rhythm_len == 0) return 0;

    int count = 0;
    for (int p = 0; p < AR_NUM_PATTERNS && count < max_results; p++) {
        int resampled[AR_MAX_GRID];
        resample_grid(patterns_storage[p].grid, patterns_storage[p].grid_len,
                      resampled, rhythm_len);
        double score = circular_correlation(rhythm_grid, rhythm_len,
                                            resampled, rhythm_len);
        if (score >= pm->match_threshold) {
            results[count].pattern = &patterns_storage[p];
            results[count].score = score;
            count++;
        }
    }

    /* Sort by score descending (simple insertion sort) */
    for (int i = 1; i < count; i++) {
        ArMatchResult tmp = results[i];
        int j = i;
        while (j > 0 && results[j-1].score < tmp.score) {
            results[j] = results[j-1];
            j--;
        }
        results[j] = tmp;
    }

    return count;
}

const ArPattern *ar_pattern_best_match(const ArPatternMatcher *pm,
                                        const ArRhythm *r)
{
    ArMatchResult results[AR_NUM_PATTERNS];
    int n = ar_pattern_match(pm, r, results, AR_NUM_PATTERNS);
    return (n > 0) ? results[0].pattern : NULL;
}

/* ─── Syncopation detection ────────────────────────────────── */

double ar_detect_syncopation(const ArRhythm *r)
{
    if (r->num_beats < 2 || r->bpm <= 0) return 0.0;

    double beat_dur = 60.0 / r->bpm;
    double bar_dur = ar_time_signature_bar_duration(&r->time_signature, r->bpm);
    if (bar_dur <= 0) return 0.0;

    int syncopated = 0;
    for (int i = 0; i < r->num_beats; i++) {
        double pos = fmod(r->beats[i].time, bar_dur);
        double nearest = round(pos / beat_dur) * beat_dur;
        double offset = fabs(pos - nearest);
        if (offset > beat_dur * 0.1)
            syncopated++;
    }
    return (double)syncopated / (double)r->num_beats;
}

/* ─── Polyrhythm detection ─────────────────────────────────── */

int ar_detect_polyrhythm(const ArRhythm *r, ArPolyrhythm *out, int max_out)
{
    if (r->num_beats < 4) return 0;

    double intervals[AR_MAX_BEATS];
    int n_intervals;
    ar_rhythm_intervals(r, intervals, &n_intervals);
    if (n_intervals == 0) return 0;

    /* Simple approach: try integer ratios of IOI clusters */
    /* Group intervals into clusters and count */
    int count = 0;
    double mean_ioi = 0;
    for (int i = 0; i < n_intervals; i++) mean_ioi += intervals[i];
    mean_ioi /= n_intervals;

    /* Count short vs long intervals */
    int short_count = 0, long_count = 0;
    for (int i = 0; i < n_intervals; i++) {
        if (intervals[i] < mean_ioi * 0.75) short_count++;
        else if (intervals[i] > mean_ioi * 1.25) long_count++;
    }

    if (short_count > 0 && long_count > 0 && count < max_out) {
        int a = short_count, b = long_count;
        /* Reduce to simplest ratio */
        int gcd_val = a, gcd_other = b;
        while (gcd_other) { int tmp = gcd_val % gcd_other; gcd_val = gcd_other; gcd_other = tmp; }
        out[count].a = a / gcd_val;
        out[count].b = b / gcd_val;
        out[count].strength = (double)(short_count + long_count) / n_intervals;
        count++;
    }

    return count;
}

/* ─── Autocorrelation ──────────────────────────────────────── */

int ar_autocorrelation(const ArRhythm *r, int max_lags, double *out_corr)
{
    double intervals[AR_MAX_BEATS];
    int n;
    ar_rhythm_intervals(r, intervals, &n);
    if (n < 2) return 0;

    int n_lags = max_lags < (n - 1) ? max_lags : (n - 1);

    /* Compute mean */
    double mean = 0;
    for (int i = 0; i < n; i++) mean += intervals[i];
    mean /= n;

    /* Compute variance */
    double var = 0;
    for (int i = 0; i < n; i++) {
        double d = intervals[i] - mean;
        var += d * d;
    }
    if (var == 0) {
        for (int lag = 0; lag < n_lags; lag++) out_corr[lag] = 0;
        return n_lags;
    }

    for (int lag = 1; lag <= n_lags; lag++) {
        double sum = 0;
        int count = 0;
        for (int i = 0; i < n - lag; i++) {
            sum += (intervals[i] - mean) * (intervals[i + lag] - mean);
            count++;
        }
        out_corr[lag - 1] = (count > 0) ? sum / (var * count / n) : 0;
    }

    return n_lags;
}

int ar_estimate_period(const ArRhythm *r)
{
    double corr[256];
    int n_lags = ar_autocorrelation(r, 256, corr);
    if (n_lags == 0) return 0;

    int best_lag = 0;
    double best_corr = -1e30;
    for (int i = 0; i < n_lags; i++) {
        if (corr[i] > best_corr) {
            best_corr = corr[i];
            best_lag = i + 1;
        }
    }
    return best_lag;
}

/* ─── Onset detection ──────────────────────────────────────── */

int ar_detect_onsets(const double *envelope, int n_samples, double sample_rate,
                     double threshold, int *onsets, int max_onsets)
{
    if (n_samples < 2) return 0;

    int count = 0;
    double spectral_flux = 0;
    double prev = 0;

    for (int i = 1; i < n_samples && count < max_onsets; i++) {
        double diff = envelope[i] - prev;
        spectral_flux = (diff > 0) ? diff : 0;

        if (spectral_flux > threshold && envelope[i] > prev) {
            /* Minimum spacing: ~50ms */
            int ok = 1;
            if (count > 0) {
                double spacing = (double)(i - onsets[count - 1]) / sample_rate;
                if (spacing < 0.05) ok = 0;
            }
            if (ok) {
                onsets[count++] = i;
            }
        }
        prev = envelope[i];
    }

    return count;
}
