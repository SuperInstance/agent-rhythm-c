#include "agent_rhythm.h"
#include <assert.h>
#include <stdio.h>

static void test_beat(void) {
    Beat b = beat_new(1.5);
    assert(fabs(b.time - 1.5) < 0.001);
    assert(fabs(b.velocity - 1.0) < 0.001);
    Beat b2 = beat_with_velocity(b, 0.9);
    assert(fabs(b2.velocity - 0.9) < 0.001);
    assert(beat_is_accent(&b2));
    printf("  ✓ beat creation and accent\n");
}

static void test_time_signature(void) {
    TimeSignature ts = time_signature_new(4, 4);
    assert(!time_signature_is_compound(&ts));
    TimeSignature ts6 = time_signature_new(6, 8);
    assert(time_signature_is_compound(&ts6));
    double bar = time_signature_bar_duration(&ts, 120.0);
    assert(fabs(bar - 2.0) < 0.01);
    printf("  ✓ time signature (4/4 bar@120bpm=%.2fs)\n", bar);
}

static void test_steady_rhythm(void) {
    Rhythm r = rhythm_steady(16, 120.0);
    assert(r.beat_count == 16);
    assert(fabs(rhythm_duration(&r) - 7.5) < 0.01);
    assert(rhythm_density(&r) > 1.0);
    printf("  ✓ steady rhythm (16 beats, dur=%.2fs, density=%.2f)\n",
        rhythm_duration(&r), rhythm_density(&r));
}

static void test_rhythm_from_intervals(void) {
    double ivs[] = {0.5, 0.5, 0.5, 0.25, 0.25};
    Rhythm r = rhythm_from_intervals(ivs, 5, 120.0);
    assert(r.beat_count == 5);
    assert(fabs(r.beats[1].time - 0.5) < 0.001);
    assert(fabs(r.beats[4].time - 1.75) < 0.001);
    printf("  ✓ rhythm from intervals\n");
}

static void test_rhythm_from_grid(void) {
    int hits[] = {1, 0, 0, 1, 0, 0, 1, 0};
    Rhythm r = rhythm_from_grid(hits, 8, 0.25, 120.0);
    assert(r.beat_count == 3);
    printf("  ✓ rhythm from grid (tresillo: %d hits)\n", r.beat_count);
}

static void test_bar_count(void) {
    Rhythm r = rhythm_steady(16, 120.0);
    int bars = rhythm_bar_count(&r);
    assert(bars > 0);
    printf("  ✓ bar count (%d bars in 16 beats @120bpm)\n", bars);
}

static void test_cadence_steady(void) {
    double ts[20];
    for (int i = 0; i < 20; i++) ts[i] = i * 0.5;
    CadenceResult cr = cadence_analyze(ts, 20, 5.0);
    assert(cr.regularity > 0.9);
    assert(cr.std_interval < 0.001);
    printf("  ✓ cadence: regularity=%.3f, bursts=%d\n", cr.regularity, cr.burst_count);
}

static void test_cadence_few_events(void) {
    double ts[] = {1.0, 2.0};
    CadenceResult cr = cadence_analyze(ts, 2, 5.0);
    assert(cr.period < 0); /* not detected */
    printf("  ✓ cadence with too few events\n");
}

static void test_cadence_bursts(void) {
    double ts[] = {0, 0.1, 0.2, 10.0, 10.1, 10.2, 20.0};
    CadenceResult cr = cadence_analyze(ts, 7, 5.0);
    assert(cr.burst_count == 3);
    printf("  ✓ burst detection (%d bursts)\n", cr.burst_count);
}

static void test_tempo_global(void) {
    double ts[20];
    for (int i = 0; i < 20; i++) ts[i] = i * 0.5;
    double bpm = tempo_global_bpm(ts, 20);
    assert(fabs(bpm - 120.0) < 1.0);
    printf("  ✓ global BPM = %.1f\n", bpm);
}

static void test_tempo_too_few(void) {
    double ts[] = {0.0};
    assert(tempo_global_bpm(ts, 1) == 0.0);
    printf("  ✓ tempo with too few beats\n");
}

int main(void) {
    printf("=== agent-rhythm tests ===\n\n");
    test_beat();
    test_time_signature();
    test_steady_rhythm();
    test_rhythm_from_intervals();
    test_rhythm_from_grid();
    test_bar_count();
    test_cadence_steady();
    test_cadence_few_events();
    test_cadence_bursts();
    test_tempo_global();
    test_tempo_too_few();
    printf("\n✅ All 11 tests passed!\n");
    return 0;
}
