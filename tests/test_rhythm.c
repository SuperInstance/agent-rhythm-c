/*
 * Tests for agent-rhythm-c
 */
#include "agent_rhythm.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#define ASSERT_FEQ(a, b, eps) do { \
    if (fabs((a) - (b)) > (eps)) { \
        fprintf(stderr, "FAIL %s:%d: %.10f != %.10f\n", \
                __FILE__, __LINE__, (double)(a), (double)(b)); \
        return 1; \
    } \
} while(0)

static int test_steady_rhythm(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    assert(r.num_beats == 8);
    ASSERT_FEQ(r.beats[0].time, 0.0, 1e-10);
    ASSERT_FEQ(r.beats[1].time, 0.5, 1e-10);
    ASSERT_FEQ(r.beats[7].time, 3.5, 1e-10);
    printf("  PASS steady_rhythm\n");
    return 0;
}

static int test_rhythm_from_intervals(void)
{
    double intervals[] = {0.5, 0.5, 1.0, 0.5};
    ArRhythm r;
    ar_rhythm_from_intervals(&r, intervals, 4, 120.0, ar_time_signature_default());
    assert(r.num_beats == 4);
    ASSERT_FEQ(r.beats[0].time, 0.0, 1e-10);
    ASSERT_FEQ(r.beats[1].time, 0.5, 1e-10);
    ASSERT_FEQ(r.beats[2].time, 1.0, 1e-10);
    ASSERT_FEQ(r.beats[3].time, 2.0, 1e-10);
    printf("  PASS rhythm_from_intervals\n");
    return 0;
}

static int test_rhythm_from_grid(void)
{
    int hits[] = {1, 0, 0, 0, 1, 0, 0, 0};
    ArRhythm r;
    ar_rhythm_from_grid(&r, hits, 8, 0.25, 120.0, ar_time_signature_default());
    assert(r.num_beats == 2);
    ASSERT_FEQ(r.beats[0].time, 0.0, 1e-10);
    ASSERT_FEQ(r.beats[1].time, 1.0, 1e-10);
    printf("  PASS rhythm_from_grid\n");
    return 0;
}

static int test_rhythm_duration(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    ASSERT_FEQ(ar_rhythm_duration(&r), 3.5, 1e-10);
    printf("  PASS rhythm_duration\n");
    return 0;
}

static int test_rhythm_density(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    double d = ar_rhythm_density(&r);
    ASSERT_FEQ(d, 8.0 / 3.5, 1e-6);
    printf("  PASS rhythm_density (%.4f)\n", d);
    return 0;
}

static int test_intervals(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 5, 120.0, ar_time_signature_default(), 1.0);
    double iv[4];
    int n;
    ar_rhythm_intervals(&r, iv, &n);
    assert(n == 4);
    for (int i = 0; i < 4; i++)
        ASSERT_FEQ(iv[i], 0.5, 1e-10);
    printf("  PASS intervals\n");
    return 0;
}

static int test_bar_count(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 9, 120.0, ar_time_signature_default(), 1.0);
    /* 9 beats at 120bpm = 4.0s, bar = 2.0s → 2 bars */
    assert(ar_rhythm_bar_count(&r) == 2);
    printf("  PASS bar_count\n");
    return 0;
}

static int test_quantize(void)
{
    double intervals[] = {0.24, 0.26, 0.51, 0.49};
    ArRhythm r;
    ar_rhythm_from_intervals(&r, intervals, 4, 120.0, ar_time_signature_default());
    ar_rhythm_quantize(&r, 0.25);
    ASSERT_FEQ(r.beats[0].time, 0.0, 1e-10);
    ASSERT_FEQ(r.beats[1].time, 0.25, 1e-10);
    ASSERT_FEQ(r.beats[2].time, 0.5, 1e-10);
    ASSERT_FEQ(r.beats[3].time, 1.0, 1e-10);
    printf("  PASS quantize\n");
    return 0;
}

static int test_stretch(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 4, 120.0, ar_time_signature_default(), 1.0);
    ar_rhythm_stretch(&r, 2.0);
    ASSERT_FEQ(r.beats[3].time, 3.0, 1e-10);  /* was 1.5, ×2 = 3.0 */
    ASSERT_FEQ(r.bpm, 60.0, 1e-10);
    printf("  PASS stretch\n");
    return 0;
}

static int test_time_signature(void)
{
    ArTimeSignature ts = {6, 8};
    assert(ar_time_signature_is_compound(&ts));
    ArTimeSignature ts2 = {4, 4};
    assert(!ar_time_signature_is_compound(&ts2));
    double bd = ar_time_signature_bar_duration(&ts2, 120.0);
    ASSERT_FEQ(bd, 2.0, 1e-10);
    printf("  PASS time_signature\n");
    return 0;
}

static int test_pattern_match_four_on_floor(void)
{
    /* Create a four-on-floor rhythm (quarter notes at 120bpm) */
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    ArPatternMatcher pm;
    ar_pattern_matcher_init(&pm);
    ArMatchResult results[AR_NUM_PATTERNS];
    int n = ar_pattern_match(&pm, &r, results, AR_NUM_PATTERNS);
    assert(n >= 1);
    assert(results[0].score >= 0.7);  /* Should match something well */
    printf("  PASS pattern_match (matched %d patterns, best=%s score=%.3f)\n",
           n, results[0].pattern->name, results[0].score);
    return 0;
}

static int test_best_match(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    ArPatternMatcher pm;
    ar_pattern_matcher_init(&pm);
    const ArPattern *p = ar_pattern_best_match(&pm, &r);
    assert(p != NULL);
    printf("  PASS best_match (%s)\n", p->name);
    return 0;
}

static int test_syncopation_steady(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 8, 120.0, ar_time_signature_default(), 1.0);
    double s = ar_detect_syncopation(&r);
    assert(s < 0.1);  /* Steady rhythm should have low syncopation */
    printf("  PASS syncopation_steady (%.3f)\n", s);
    return 0;
}

static int test_syncopation_offbeat(void)
{
    double intervals[] = {0.75, 0.25, 0.75, 0.25, 0.75, 0.25};
    ArRhythm r;
    ar_rhythm_from_intervals(&r, intervals, 6, 120.0, ar_time_signature_default());
    double s = ar_detect_syncopation(&r);
    assert(s > 0.3);  /* Off-beat rhythm should have high syncopation */
    printf("  PASS syncopation_offbeat (%.3f)\n", s);
    return 0;
}

static int test_autocorrelation(void)
{
    /* Steady rhythm: autocorrelation should peak at regular intervals */
    ArRhythm r;
    ar_rhythm_steady(&r, 16, 120.0, ar_time_signature_default(), 1.0);
    double corr[32];
    int n = ar_autocorrelation(&r, 32, corr);
    assert(n > 0);
    printf("  PASS autocorrelation (n_lags=%d, first_corr=%.4f)\n", n, corr[0]);
    return 0;
}

static int test_estimate_period(void)
{
    ArRhythm r;
    ar_rhythm_steady(&r, 16, 120.0, ar_time_signature_default(), 1.0);
    int period = ar_estimate_period(&r);
    assert(period >= 1);
    printf("  PASS estimate_period (period=%d)\n", period);
    return 0;
}

static int test_onset_detection(void)
{
    /* Create envelope with 3 peaks */
    double envelope[1000];
    for (int i = 0; i < 1000; i++) envelope[i] = 0.1;
    envelope[100] = 1.0;
    envelope[300] = 0.9;
    envelope[600] = 0.8;

    int onsets[10];
    int n = ar_detect_onsets(envelope, 1000, 1000.0, 0.3, onsets, 10);
    assert(n >= 2);
    printf("  PASS onset_detection (%d onsets)\n", n);
    return 0;
}

static int test_beat_helpers(void)
{
    ArBeat accent = {0.0, 0.9, 0.0};
    ArBeat normal = {0.5, 0.5, 0.0};
    ArBeat rest   = {1.0, 0.0, 0.0};
    assert(ar_beat_is_accent(&accent));
    assert(!ar_beat_is_accent(&normal));
    assert(ar_beat_is_rest(&rest));
    assert(!ar_beat_is_rest(&normal));
    printf("  PASS beat_helpers\n");
    return 0;
}

/* ─── Main ─────────────────────────────────────────────────── */

typedef int (*test_fn)(void);

int main(void)
{
    test_fn tests[] = {
        test_steady_rhythm,
        test_rhythm_from_intervals,
        test_rhythm_from_grid,
        test_rhythm_duration,
        test_rhythm_density,
        test_intervals,
        test_bar_count,
        test_quantize,
        test_stretch,
        test_time_signature,
        test_pattern_match_four_on_floor,
        test_best_match,
        test_syncopation_steady,
        test_syncopation_offbeat,
        test_autocorrelation,
        test_estimate_period,
        test_onset_detection,
        test_beat_helpers,
    };
    int n = sizeof(tests) / sizeof(tests[0]);
    int failures = 0;
    printf("Running %d tests...\n", n);
    for (int i = 0; i < n; i++) {
        if (tests[i]()) failures++;
    }
    printf("\n%s: %d/%d passed\n",
           failures ? "FAIL" : "OK", n - failures, n);
    return failures;
}
