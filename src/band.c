// ============================================================
//  GAIN BOY - drum solo, synth, FX, pad effects (ROM bank 0)
//  Power-ups that change the sound, loop timer, fuzzball bite,
//  chord progressions, and the drum kit on the floor. Where you
//  land is still what plays.
// ============================================================
#include "game.h"
#include "gig.h"
#include "band.h"

const uint8_t song_minor[6] = { 0, 1, 1, 0, 1, 1 };   // SUNNY NIGHT RAVE DREAM FUNK SPOOKY
uint8_t inst = I_MELODY;
uint8_t dloop[32], bloop[32], dloop_on = 0, bloop_on = 0;
const char * const inst_names[3] = { "MELODY", "DRUM SOLO", "SYNTH!" };

/* DRUM SOLO kit on the 8 floor keys (low→high). Each hit layers CH1 + noise
 * so it reads on small speakers — see sound.c for the primitive voices. */
void drum_hit(uint8_t k) {
    switch (k) {
        case 0: /* KICK   */ kick(); noise_hit(62, 0xD1, 0x00); break;
        case 1: /* LOW TOM*/ ch1_raw(1250, 0x1B, 2, 0xF2); noise_hit(60, 0xA1, 0x31); break;
        case 2: /* SNARE  */ snare(); clap_frames = 3; break;
        case 3: /* CLAP   */ noise_hit(48, 0xD1, 0x12); clap_frames = 3; break;
        case 4: /* HAT    */ noise_hit(58, 0x91, 0x00); break;
        case 5: /* OPEN HT*/ noise_hit(30, 0xA3, 0x00); break;
        case 6: /* COWBELL*/ noise_hit(50, 0xB2, 0x5B); ch1_raw(1900, 0x00, 1, 0xA2); break;
        default:/* CRASH  */ kick(); noise_hit(4, 0xF4, 0x01); break;
    }
}
// ---- the DRUM SOLO: once per gig, short and loud ----
uint8_t solo_used = 0;
void solo_hit(void) {                  // every landing in a solo: the stage thumps, stars fly, the crowd jumps
    shake = 4; if (energy < 28) energy += 3;
    spawn_star(px - 2, 108); spawn_star(px + 10, 108);
}
// ---- the fuzzball bites: a sour note, points lost, the crowd goes quiet ----
uint8_t hurt_cool = 0;
void fuzz_ouch(void) {
    if (hurt_cool || attract) return;
    hurt_cool = 60;
    lead(key_note(1) + 1, 0xF1, 0);                                    // a nasty clash...
    ch1_raw(700, 0x7F, 3, 0xD3); noise_hit(40, 0xC3, 0x77);           // ...a buzz and a rasp
    show_banner("OUCH!", 0, 10); shake = 8; energy = 0;
    sub_score(15);
    bad_on = 0; bad_wait = 5 + (rand() & 3); move_sprite(12, 0, 0);   // it runs off stage: one bite per visit
}
void fuzz_touch(void) {               // walking into it (standing mode) bites too
    if (hurt_cool) return;
    if (!bad_on || bad_squash) return;
    int16_t d = (int16_t)(px + 8) - (int16_t)(bad_x + 4);
    if (d > -9 && d < 9) fuzz_ouch();
}
// the bass: the same keys, two octaves down, on the wave channel
// SYNTH: a bright wave voice one octave under the lead (audible on small speakers, unlike a sub bass)
void bass_hit(uint8_t k, uint8_t fps) {
    use_wave(6);
    wave_note(key_note(k) - 12, 256 - fps * 9, 0x20);
}

// ---------------- stars: what fits right now ----------------
// drums: the next landing's job. kick on beats 1 and 3, snare/clap on 2 and 4, hats in between
uint8_t drum_mask(uint8_t t) {
    t &= 15;
    if ((t & 7) == 0) return 0x03;      // kick / low tom
    if ((t & 7) == 4) return 0x0C;      // snare / clap
    return 0x30;                        // hats
}
// bass: the root and fifth of the chord (what a bassist leans on)
int8_t bm_ch = 127; uint8_t bm_v = 255, bm_cache = 0;      // only recompute when the chord changes
uint8_t bass_mask(void) {
    const uint8_t *off = song_minor[song] ? key_off_min : key_off_maj;
    int8_t ch = chord_semi();
    if (ch == bm_ch && song == bm_v) return bm_cache;
    bm_ch = ch; bm_v = song;
    uint8_t r = (uint8_t)((ch + 24) % 12), f = (uint8_t)((ch + 31) % 12), mask = 0;
    for (uint8_t k = 0; k < 8; k++) { uint8_t pc = off[k] >= 12 ? off[k] - 12 : off[k]; if (pc == r || pc == f) mask |= 1 << k; }
    bm_cache = mask;
    return mask;
}
// the star mask for the instrument being played, for a landing at step t
uint8_t inst_mask(uint8_t t, uint8_t melody_mask) {
    if (inst == I_DRUMS) return drum_mask(t);
    if (inst == I_BASS) return melody_mask;                  // SYNTH plays the melody keys: the same stars
    return melody_mask;
}

// ---------------- switching ----------------
uint8_t inst_bars = 0;                  // bars of the instrument power-up left
uint8_t fx_id = 0, fx_bars = 0;         // FX power-up: 1 LONG NOTES, 2 ECHO, 3 WOBBLE, 4 SPARKLE
const char * const fx_names[5] = { "", "LONG NOTE", "ECHO!", "WOBBLE!", "SPARKLE!" };
void fx_power(void) { fx_id = 1 + (rand() & 3); fx_bars = 5; show_banner(fx_names[fx_id], 0, 14); }
// each chord pad colours your playing for 2 bars: ECHO, WOBBLE, LONG NOTES, SPARKLE
const uint8_t pad_fx[4] = { 2, 3, 1, 4 };
void fx_pad(uint8_t p) { if (fx_id != pad_fx[p] || fx_bars < 2) { fx_id = pad_fx[p]; fx_bars = 3; show_banner(fx_names[fx_id], 0, 10); } else fx_bars = 3; }
void inst_power(uint8_t i) {            // DRUM SOLO: the rest of this bar + 2 more (once per gig); SYNTH: + 4
    inst = i; inst_bars = (i == I_DRUMS) ? 3 : 5;
    if (i == I_DRUMS) solo_used = 1;
    show_banner(inst_names[i], 0, 14);
    hud_dirty |= 8;
}
// ---- loops last 4 bars, then stop by themselves (SELECT stops them early) ----
uint8_t loop_bars = 0, loop_was = 0, key_up = 0;
void band_bar(void) {                   // called on every bar line
    uint8_t any = loop_on || dloop_on || bloop_on;
    if (any && !loop_was) loop_bars = 5;                 // just started: the rest of this bar + 4 more
    if (any && loop_bars && --loop_bars == 0) {
        loop_on = 0; dloop_on = 0; bloop_on = 0; NR32_REG = 0;
        show_banner("LOOP OFF", 0, 10); any = 0;
    }
    if (any != loop_was || loop_bars == 1) hud_dirty |= 16;
    loop_was = any;
    if (inst_bars && --inst_bars == 0) { inst = I_MELODY; show_banner("MELODY", 0, 10); hud_dirty |= 8; }
    if (fx_bars && --fx_bars == 0) fx_id = 0;
}
void inst_set(uint8_t i) { if (inst != i) { inst = i; inst_bars = 0; show_banner(inst_names[i], 0, 12); hud_dirty |= 8; } }
char inst_letter(void) { return inst == I_DRUMS ? 'D' : inst == I_BASS ? 'S' : 'M'; }

// ---------------- loops for the band ----------------
// returns 1 if the capture was handled here (drums or bass), 0 for melody (main.c handles it)
uint8_t band_capture(const uint8_t *hist) {
    uint8_t i, n = 0;
    if (inst == I_MELODY) { bloop_on = 0; return 0; }            // a melody loop takes the wave channel back
    uint8_t *dst = (inst == I_DRUMS) ? dloop : bloop;
    for (i = 0; i < 32; i++) { uint8_t e = hist[i]; dst[i] = (e && !(e & 0x40)) ? (e & 0x0F) : 0; if (dst[i]) n++; }
    if (!n) return 1;
    if (inst == I_DRUMS) { dloop_on = 1; show_banner("BEAT LOOP", 0, 16); }
    else { bloop_on = 1; loop_on = 0; show_banner("SYNTHLOOP", 0, 16); }   // the bass loop owns the wave channel
    return 1;
}
void band_clear(void) { dloop_on = 0; bloop_on = 0; inst_bars = 0; fx_id = 0; fx_bars = 0; solo_used = 0; hurt_cool = 0; loop_bars = 0; loop_was = 0; key_up = 0; }
// play the band's loops for this 16th. returns 1 if a drum sound used the noise channel
uint8_t band_step(uint8_t pos, uint8_t muted, uint8_t fps) {
    uint8_t used = 0;
    if (inst_bars == 1 && !(pos & 3)) hud_dirty |= 8;     // last bar of the power-up: the letter blinks
    if (loop_bars == 1 && !(pos & 3)) hud_dirty |= 16;    // last bar of the loop: the L blinks
    if (muted) return 0;
    if (dloop_on && dloop[pos]) { drum_hit(dloop[pos] - 1); used = 1; }
    if (bloop_on && bloop[pos] && !loop_on) bass_hit(bloop[pos] - 1, fps);
    return used;
}

// ---------------- chord progressions: a fresh one for every gig ----------------
// four per mode; the song's own (major songs SUNNY/DREAM, minor songs NIGHT/RAVE) plus three more
int8_t cur_prog[4] = {0, 7, 9, 5};
uint8_t cur_pmask = 0x04;
const int8_t pool_maj[4][4] = { {0, 7, 9, 5}, {0, 9, 5, 7}, {9, 5, 0, 7}, {0, 5, 9, 7} };   // I-V-vi-IV, I-vi-IV-V, vi-IV-I-V, I-IV-vi-V
const uint8_t mask_maj[4]   = { 0x04, 0x02, 0x01, 0x04 };
const int8_t pool_min[4][4] = { {0, 8, 3, 10}, {0, 5, 10, 3}, {0, 10, 8, 10}, {0, 3, 8, 10} }; // i-VI-III-VII, i-iv-VII-III, i-VII-VI-VII, i-III-VI-VII
const uint8_t mask_min[4]   = { 0x01, 0x03, 0x01, 0x01 };
void pick_prog(void) {
    uint8_t p = rand() & 3, minor = song_minor[song];
    for (uint8_t i = 0; i < 4; i++) cur_prog[i] = minor ? pool_min[p][i] : pool_maj[p][i];
    cur_pmask = minor ? mask_min[p] : mask_maj[p];
}
