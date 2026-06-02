#include "agent_rhythm.h"
#include <assert.h>
#include <stdio.h>

static int test_count = 0;
#define TEST(name) do { printf("  ✓ %s\n", name); test_count++; } while(0)

/* ── Original tests (11) ───────────────────────────────────────────── */

static void test_beat(void) {
    Beat b = beat_new(1.5);
    assert(fabs(b.time - 1.5) < 0.001);
    assert(fabs(b.velocity - 1.0) < 0.001);
    Beat b2 = beat_with_velocity(b, 0.9);
    assert(fabs(b2.velocity - 0.9) < 0.001);
    assert(beat_is_accent(&b2));
    TEST("beat creation and accent");
}

static void test_time_signature(void) {
    TimeSignature ts = time_signature_new(4, 4);
    assert(!time_signature_is_compound(&ts));
    TimeSignature ts6 = time_signature_new(6, 8);
    assert(time_signature_is_compound(&ts6));
    double bar = time_signature_bar_duration(&ts, 120.0);
    assert(fabs(bar - 2.0) < 0.01);
    TEST("time signature (4/4 bar@120bpm=2.0s)");
}

static void test_steady_rhythm(void) {
    Rhythm r = rhythm_steady(16, 120.0);
    assert(r.beat_count == 16);
    assert(fabs(rhythm_duration(&r) - 7.5) < 0.01);
    assert(rhythm_density(&r) > 1.0);
    TEST("steady rhythm (16 beats)");
}

static void test_rhythm_from_intervals(void) {
    double ivs[] = {0.5, 0.5, 0.5, 0.25, 0.25};
    Rhythm r = rhythm_from_intervals(ivs, 5, 120.0);
    assert(r.beat_count == 5);
    assert(fabs(r.beats[1].time - 0.5) < 0.001);
    assert(fabs(r.beats[4].time - 1.75) < 0.001);
    TEST("rhythm from intervals");
}

static void test_rhythm_from_grid(void) {
    int hits[] = {1, 0, 0, 1, 0, 0, 1, 0};
    Rhythm r = rhythm_from_grid(hits, 8, 0.25, 120.0);
    assert(r.beat_count == 3);
    TEST("rhythm from grid (tresillo)");
}

static void test_bar_count(void) {
    Rhythm r = rhythm_steady(16, 120.0);
    int bars = rhythm_bar_count(&r);
    assert(bars > 0);
    TEST("bar count");
}

static void test_cadence_steady(void) {
    double ts[20];
    for (int i = 0; i < 20; i++) ts[i] = i * 0.5;
    CadenceResult cr = cadence_analyze(ts, 20, 5.0);
    assert(cr.regularity > 0.9);
    assert(cr.std_interval < 0.001);
    TEST("cadence: steady rhythm");
}

static void test_cadence_few_events(void) {
    double ts[] = {1.0, 2.0};
    CadenceResult cr = cadence_analyze(ts, 2, 5.0);
    assert(cr.period < 0);
    TEST("cadence with too few events");
}

static void test_cadence_bursts(void) {
    double ts[] = {0, 0.1, 0.2, 10.0, 10.1, 10.2, 20.0};
    CadenceResult cr = cadence_analyze(ts, 7, 5.0);
    assert(cr.burst_count == 3);
    TEST("burst detection (3 bursts)");
}

static void test_tempo_global(void) {
    double ts[20];
    for (int i = 0; i < 20; i++) ts[i] = i * 0.5;
    double bpm = tempo_global_bpm(ts, 20);
    assert(fabs(bpm - 120.0) < 1.0);
    TEST("global BPM = 120");
}

static void test_tempo_too_few(void) {
    double ts[] = {0.0};
    assert(tempo_global_bpm(ts, 1) == 0.0);
    TEST("tempo with too few beats");
}

/* ── New tests (target: 22+) ──────────────────────────────────────── */

static void test_beat_with_duration(void) {
    Beat b = beat_new(1.0);
    b = beat_with_duration(b, 0.5);
    assert(fabs(b.duration - 0.5) < 1e-10);
    TEST("beat with duration");
}

static void test_beat_eq(void) {
    Beat a = beat_new(1.0);
    Beat b = beat_new(1.0);
    Beat c = beat_new(2.0);
    assert(beat_eq(&a, &b));
    assert(!beat_eq(&a, &c));
    assert(!beat_eq(NULL, &a));
    TEST("beat equality");
}

static void test_beat_is_accent_threshold(void) {
    Beat b = beat_with_velocity(beat_new(0.0), 0.79);
    assert(!beat_is_accent(&b));
    Beat b2 = beat_with_velocity(beat_new(0.0), 0.80);
    assert(beat_is_accent(&b2));
    assert(!beat_is_accent(NULL));
    TEST("beat accent threshold");
}

static void test_time_signature_simple(void) {
    TimeSignature ts3 = time_signature_new(3, 4);
    assert(!time_signature_is_compound(&ts3));
    assert(time_signature_is_simple(&ts3));
    TimeSignature ts4 = time_signature_new(4, 4);
    assert(time_signature_is_simple(&ts4));
    assert(!time_signature_is_simple(NULL));
    TEST("time signature simple/compound");
}

static void test_time_signature_null(void) {
    assert(time_signature_bar_duration(NULL, 120.0) == 0.0);
    assert(time_signature_bar_duration(NULL, 0.0) == 0.0);
    TEST("time signature null safety");
}

static void test_rhythm_from_intervals_null(void) {
    Rhythm r = rhythm_from_intervals(NULL, 5, 120.0);
    assert(r.beat_count == 0);
    TEST("rhythm from null intervals");
}

static void test_rhythm_from_grid_null(void) {
    Rhythm r = rhythm_from_grid(NULL, 8, 0.25, 120.0);
    assert(r.beat_count == 0);
    TEST("rhythm from null grid");
}

static void test_rhythm_steady_edge(void) {
    Rhythm r0 = rhythm_steady(0, 120.0);
    assert(r0.beat_count == 0);
    Rhythm rn = rhythm_steady(-5, 120.0);
    assert(rn.beat_count == 0);
    Rhythm rb = rhythm_steady(10, 0.0);
    assert(rb.beat_count == 0);
    TEST("rhythm steady edge cases");
}

static void test_rhythm_intervals(void) {
    double ivs[] = {0.5, 0.5, 0.5, 0.5};
    Rhythm r = rhythm_from_intervals(ivs, 4, 120.0);
    assert(r.beat_count == 4);
    double out[4];
    int n = rhythm_intervals(&r, out, 4);
    assert(n == 3);
    assert(fabs(out[0] - 0.5) < 1e-10);
    assert(fabs(out[1] - 0.5) < 1e-10);
    assert(fabs(out[2] - 0.5) < 1e-10);
    TEST("rhythm_intervals extraction");
}

static void test_rhythm_intervals_null(void) {
    Rhythm r = rhythm_steady(4, 120.0);
    assert(rhythm_intervals(&r, NULL, 4) == 0);
    double out[4];
    assert(rhythm_intervals(NULL, out, 4) == 0);
    TEST("rhythm_intervals null safety");
}

static void test_rhythm_mean_interval(void) {
    Rhythm r = rhythm_steady(5, 120.0);
    double mean = rhythm_mean_interval(&r);
    assert(fabs(mean - 0.5) < 1e-10);
    TEST("rhythm mean interval");
}

static void test_rhythm_std_interval(void) {
    Rhythm r = rhythm_steady(5, 120.0);
    double std = rhythm_std_interval(&r);
    assert(fabs(std) < 1e-10);
    TEST("rhythm std interval (steady = 0)");
}

static void test_rhythm_mean_interval_edge(void) {
    Rhythm r = rhythm_steady(1, 120.0);
    assert(rhythm_mean_interval(&r) == 0.0);
    assert(rhythm_mean_interval(NULL) == 0.0);
    TEST("rhythm mean interval edge cases");
}

static void test_rhythm_duration_edge(void) {
    Rhythm r = rhythm_steady(1, 120.0);
    assert(rhythm_duration(&r) == 0.0);
    assert(rhythm_duration(NULL) == 0.0);
    TEST("rhythm duration edge cases");
}

static void test_cadence_null(void) {
    CadenceResult cr = cadence_analyze(NULL, 10, 5.0);
    assert(cr.period == -1);
    double ts[] = {1.0, 2.0, 3.0};
    cr = cadence_analyze(ts, 3, 5.0);
    assert(cr.period == -1);
    TEST("cadence null/edge cases");
}

static void test_tempo_null(void) {
    assert(tempo_global_bpm(NULL, 10) == 0.0);
    TEST("tempo null safety");
}

static void test_rhythm_swing(void) {
    Rhythm r = rhythm_steady(4, 120.0);
    /* At 120 BPM, interval = 0.5s */
    assert(fabs(r.beats[0].time - 0.0) < 1e-10);
    assert(fabs(r.beats[1].time - 0.5) < 1e-10);
    assert(fabs(r.beats[2].time - 1.0) < 1e-10);

    Rhythm swung = rhythm_swing(&r, 0.5);
    /* Beat 0 stays, beat 1 gets delayed, beat 2 stays, beat 3 gets delayed */
    assert(fabs(swung.beats[0].time - 0.0) < 1e-10);
    assert(swung.beats[1].time > 0.5);
    assert(fabs(swung.beats[2].time - 1.0) < 1e-10);
    assert(swung.beats[3].time > 1.5);
    TEST("rhythm swing");
}

static void test_rhythm_swing_zero(void) {
    Rhythm r = rhythm_steady(4, 120.0);
    Rhythm swung = rhythm_swing(&r, 0.0);
    /* No swing = same times */
    for (int i = 0; i < r.beat_count; i++) {
        assert(fabs(swung.beats[i].time - r.beats[i].time) < 1e-10);
    }
    TEST("rhythm swing (zero = no change)");
}

static void test_bar_count_edge(void) {
    Rhythm r = rhythm_steady(1, 120.0);
    assert(rhythm_bar_count(&r) == 0);
    assert(rhythm_bar_count(NULL) == 0);
    TEST("bar count edge cases");
}

static void test_benchmark_cadence(void) {
    double elapsed = benchmark_cadence(10000);
    assert(elapsed >= 0.0);
    printf("    Cadence benchmark: 10k iterations in %.4f s\n", elapsed);
    TEST("cadence benchmark");
}

static void test_steady_rhythm_timing(void) {
    Rhythm r = rhythm_steady(4, 60.0);
    assert(fabs(r.beats[0].time - 0.0) < 1e-10);
    assert(fabs(r.beats[1].time - 1.0) < 1e-10);
    assert(fabs(r.beats[2].time - 2.0) < 1e-10);
    assert(fabs(r.beats[3].time - 3.0) < 1e-10);
    TEST("steady rhythm timing at 60 BPM");
}

/* ── Main ────────────────────────────────────────────────────────────── */

int main(void) {
    printf("=== agent-rhythm tests ===\n\n");

    /* Original */
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

    /* New */
    test_beat_with_duration();
    test_beat_eq();
    test_beat_is_accent_threshold();
    test_time_signature_simple();
    test_time_signature_null();
    test_rhythm_from_intervals_null();
    test_rhythm_from_grid_null();
    test_rhythm_steady_edge();
    test_rhythm_intervals();
    test_rhythm_intervals_null();
    test_rhythm_mean_interval();
    test_rhythm_std_interval();
    test_rhythm_mean_interval_edge();
    test_rhythm_duration_edge();
    test_cadence_null();
    test_tempo_null();
    test_rhythm_swing();
    test_rhythm_swing_zero();
    test_bar_count_edge();
    test_benchmark_cadence();
    test_steady_rhythm_timing();

    printf("\n✅ All %d tests passed!\n", test_count);
    return 0;
}
