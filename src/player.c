// ============================================================
//  GAIN BOY - player / landing (ROM bank 1)
//  Bounce and jump physics, land(), tricks, capture / loop.
// ============================================================
#pragma bank 1
#include "game.h"
#include "gig.h"
#include "band.h"


// ============================================================
//  where am I standing?
// ============================================================
uint8_t cx(void) { return px + 8; }
int8_t pad_under(uint8_t x) {
    for (uint8_t p = 0; p < 4; p++) if (x + 3 >= pad_x[p] && x < pad_x[p] + 19) return p;   // a little forgiving at the edges
    return -1;
}
// surface the player can land on, given the highest point of the current bounce
uint8_t surface(uint8_t x, uint8_t peak, uint8_t *kind) {
    if (x < 16 || x >= 144) { *kind = SURF_LEDGE; return H_LEDGE; }
    if (!drop_through && pad_under(x) >= 0 && peak >= H_PAD && (dbl || b_from >= H_PAD)) { *kind = SURF_PAD; return H_PAD; }
    *kind = SURF_FLOOR; return 0;
}
uint8_t frames_of(uint8_t from_step, uint8_t n) {
    const Song *v = &songs[song];
    uint8_t f = 0;
    for (uint8_t i = 0; i < n; i++) { uint8_t s = (from_step + i) & 15; f += (s & 1) ? FPS - v->swing : FPS + v->swing; }
    return f;
}
// ONE gravity for every jump: the arc height a jump of T frames reaches is G*T*T/8 (G ~ 0.42 px/frame^2).
uint8_t arc_for(uint8_t T) { uint16_t h = ((uint16_t)T * T * 27) >> 9; return h > 90 ? 90 : (uint8_t)h; }
void start_bounce(uint8_t len) {
    b_len = len; b_left = len; b_t = 0;
    b_from = height;
    b_H = (len == 1) ? 5 : (len == 2 ? 12 : 0);
    b_T = frames_of(step, len);
    if (len == 4) { b_H = arc_for(b_T); if (b_H > 38) b_H = 38; }   // big jump off a ledge: same gravity
    pmode = M_BOUNCE;
    is_big = (len == 4); dbl = 0; tgt_s = b_from;
}
// frames left until the next step boundary, as seen by the bounce timer
uint8_t f2b(void) { return step_len - frame_in_step - 1; }
// extend the bounce in mid-air, still landing exactly on the grid
// jump from where you are now: pick the landing 16th whose natural arc is closest to the height wanted,
// so it always feels like the same weighty boombox and still lands exactly on the grid
void extend_bounce(uint8_t extra_steps, uint8_t rise) {
    uint8_t best_n = 1, best_d = 255, n, T, h, d;
    (void)extra_steps;
    for (n = 1; n <= 6; n++) {
        T = f2b() + frames_of((step + 1) & 15, n - 1);
        if (T < 10) continue;                              // never a squeezed, snappy hop
        h = arc_for(T);
        d = (h > rise) ? h - rise : (rise - h) * 2;         // falling short is worse than a bit high
        if (d < best_d) { best_d = d; best_n = n; }
    }
    b_from = height; b_t = 0; tgt_s = height; b_left = best_n;
    b_T = f2b() + frames_of((step + 1) & 15, best_n - 1);
    h = arc_for(b_T);
    b_H = (uint8_t)(((uint16_t)h * 3 + rise) >> 2);       // mostly the natural arc, nudged towards the target
    if (b_H < rise) b_H = rise;
}

// the double jump: a quick, punchy hop that peaks just above the pads (no drifting down onto them)
void dbl_jump(void) {
    if (height + 12 >= H_PAD) { extend_bounce(2, H_PAD + 12 > height ? H_PAD + 12 - height : 12); return; }   // already up high
    uint8_t need = H_PAD + 2 - height;                          // enough to count as reaching the pad
    uint8_t wall = (cx() < 16 || cx() >= 144);                  // at the walls: up to the switch box instead
    uint8_t H = (uint8_t)((wall ? DJ_WALL : DJ_APEX) - height) - (wall ? 0 : ((uint8_t)(H_PAD - height) >> 2));
    if (H < need) H = need;
    uint8_t best_n = 2, best_d = 255, n, T, d;
    for (n = 1; n <= 4; n++) {                                  // as short as the beat allows: about 20 frames
        T = f2b() + frames_of((step + 1) & 15, n - 1);
        if (T < DJ_MIN_T) continue;
        d = T > DJ_T ? T - DJ_T : DJ_T - T;
        if (d < best_d) { best_d = d; best_n = n; }
    }
    b_from = height; b_t = 0; tgt_s = height; b_left = best_n;
    b_T = f2b() + frames_of((step + 1) & 15, best_n - 1);
    b_H = H;
}
void award_star(uint8_t pts) {                             // a big moment: points, cheer, fanfare
    energy = 24; cheered = 0;
    for (uint8_t i = 0; i < 4; i++) spawn_star(16 + (rand() & 127), 124);   // the crowd throws stars
    start_sfx(4);
    shake = 10;
    add_score(pts);
    show_banner("YEAH!", 0, 12);
}
uint8_t last_trick = 0;
void do_trick(const char *name, uint8_t x, uint8_t id) {
    if (!ban_t) show_banner(name, x, 8);
    energy += 4; if (energy > 30) energy = 30;
    spawn_star(px + 4, 84); spawn_star(px - 6, 92);
    last_trick = id;
    if (tricks & (1 << id)) return;                      // already done this combo: sounds great, scores nothing
    tricks |= (1 << id);
    add_score(12);
    if (tricks == 0x3E) { tricks = 0; award_star(60); show_banner("COMBO!", 0, 14); }   // all five tricks: COMBO!
}
// ============================================================
//  landing = playing
// ============================================================
void land(uint8_t pos) {
    uint8_t kind; uint8_t h = surface(cx(), b_from + b_H, &kind);
    if (kind == SURF_PAD && vis_h + 12 < H_PAD) { kind = SURF_FLOOR; h = 0; }   // came from below: no pop-up
    drop_through = 0; from_pad = 0;
    height = h; land_kind = kind;
    landed_now = 0;
    if (kind != SURF_FLOOR) { last_k = prev_k = 255; up_run = dn_run = same_run = trill_run = 0; }
    if (kind == SURF_LEDGE) { pmode = M_STAND; ch1_raw(1000, 0x1B, 2, 0x51); return; }   // soft thock, then silence
    energy += (energy < 30) ? 1 : 0;
    if (kind == SURF_PAD) {
        uint8_t p = (uint8_t)pad_under(cx());
        start_sfx(7); fx_pad(p);                       // a harp run up the new chord, and the pad's effect for 2 bars
        pad_chord = p; override_bars = 4;
        if (!attract) {                                    // secret: pads 1-2-3-4 in a row = KEY UP!
            if (p == pad_seq) pad_seq++; else pad_seq = (p == 0) ? 1 : 0;
            if (pad_seq == 4) { pad_seq = 0; kshift = (kshift + 1) % 6; show_banner("KEY UP!", 52, 16); start_sfx(4); energy = 25; add_score(30); }
        }
        if (lit_pad != 255 && lit_pad != p) draw_pad(lit_pad, 0);
        draw_pad(p, 1); lit_pad = p; lit_pad_timer = 6;
        landed_now = 0x40 | (p + 1);
        calc_glow();
        if (p != lit_pad) add_score(2);
        spawn_spark(px + 4, 36);
        return;
    }
    // floor: the fuzzball. jump on it to squash it; land on it with a normal bounce and it bites
    if (bad_on && !bad_squash) {
        int16_t d = (int16_t)cx() - (int16_t)(bad_x + 4);
        if (d > -10 && d < 10 && !is_big) { fuzz_ouch(); return; }
        if (d > -10 && d < 10) {
            switch (rand() % 5) {                       // a random mini drum kit
                case 0: kick(); snare(); crash(); break;
                case 1: roll_steps = 4; ch1_raw(1200, 0x1C, 2, 0xC2); break;            // tom roll
                case 2: noise_hit(50, 0xB2, 0x5B); ch1_raw(1900, 0x00, 1, 0x82); break; // cowbell
                case 3: snare(); clap_frames = 3; noise_hit(46, 0xC1, 0x12); break;      // clap
                default: ch1_raw(2000, 0x7F, 0, 0xD3); noise_hit(56, 0x61, 0x00); break; // laser zap
            }
            bad_squash = 24; energy += 3; add_score(6);
            spawn_spark(bad_x, 100);
            return;
        }
    }
    uint8_t k = (cx() - 16) >> 4; if (k >= NKEYS) k = NKEYS - 1;
    if (k == mys_key) {                                  // ? key: a surprise
        switch (rand() & 3) {
            case 0: start_sfx(6); mys_last = k; break;                  // sparkly run up
            case 1: ch1_raw(2000, 0x7F, 0, 0xD3); noise_hit(56, 0x61, 0x00); break;    // laser
            case 2: ch1_raw(1950, 0x7F, 1, 0xF4); break;                                // slow dive bomb
            default: ch1_raw(1500, 0x24, 2, 0xC2); clap_frames = 3; break;              // "yeah!" chirp
        }
        mys_key = 255; mys_wait = 5 + (rand() & 3); move_sprite(SPR_MYS, 0, 0); add_score(8);
        spawn_spark(px + 4, 96); spawn_spark(px + 8, 90); energy += 3;
        return;
    }
    uint8_t m = key_note(k);
    uint8_t accent = is_big;
    uint8_t up = ((keys & J_UP) && !attract) ? 12 : 0;   // UP = sing an octave higher
    m += up;
    stab_frames = 0;
    if (inst == 1) { drum_hit(k); solo_hit(); }                              // DRUM SOLO: every landing is a big hit
    else {                                                // MELODY (and SYNTH: the same notes, doubled)
        lead(accent ? m + 12 : m, accent ? 0xD4 : songs[song].lead_env, 0);
        if (accent) { crash(); shake = 4; }
        echo_push(accent ? m + 12 : m);
        if (inst == 2) bass_hit(k, songs[song].fps);      // SYNTH: a bright wave voice an octave under your lead
        if (fx_id == 4 && !power_steps) {                // SPARKLE: an octave shimmer on every note
            stab_x[0] = nfreq(m); stab_x[1] = nfreq(m + 12); stab_x[2] = nfreq(m + 24); strum_dir = 0; stab_frames = 10;
        }
    }
    if (power_steps) {                                   // STAR POWER: every note becomes a sparkly chord
        stab_x[0] = nfreq(m + 12); stab_x[1] = nfreq(key_note(k + 2) + up + 12); stab_x[2] = nfreq(key_note(k + 4) + up + 12);
        strum_dir = 0; stab_frames = 14; energy += 2;
        spawn_spark(px, 86 - k * 3);
    }
    bar_notes[step] = k;                                  // remembered for RIFF!
    mood_note(k);                                         // the crowd hears every note: variety pleases them
    if (!attract) {
        uint8_t smask = inst_mask(step, glow_mask);
        uint8_t on_time = inst == 1 ? !(step & 1) : !(step & 3);       // drums: every 8th counts; notes: the beat
        if (((smask >> k) & 1) && on_time && b_len != 1 && (k != last_sweet_k || inst == 1) && sweet_bar < 4) {   // a sweet note
            add_score(step == 0 ? 6 : 4);                                    // the downbeat counts most
            sweet_bar++; last_sweet_k = k;
            spawn_star(px + 4, 98); spawn_star(px + 4, 108);
            if (sweet_named < 3 && !ban_t) { sweet_named++; show_banner("SWEET!", 0, 6); }
            if (A_BIG && !loop_on && inst == 0) { use_wave(2); wave_note(m - 12, 256 - songs[song].fps * 8, 0x20); }   // chorus: the crowd sings it with you
        }
        if (step == 0 && (bar & 3) == 0 && (inst == 1 ? k == 0 : (k % 5) == 0)) {    // HOME! the home note (or a kick) on the turnaround
            add_score(25); energy += 4;
            if (!ban_t) show_banner("HOME!", 0, 8);
            spawn_star(px - 4, 92); spawn_star(px + 12, 92);
            start_sfx(2);
        }
    }
    if (req_state == 2) {                                // the crowd's request
        if (k == req[req_idx]) { req_idx++; req_dirty = 1; req_stall = 0; }
        else if (req_idx > 0 && k != req[req_idx - 1]) {
            // keys on the way to the next note are fine; wandering past it restarts the tune
            uint8_t a = req[req_idx - 1], b = req[req_idx];
            uint8_t lo = a < b ? a : b, hi = a < b ? b : a;
            if (k < lo || k > hi) { req_idx = (k == req[0]) ? 1 : 0; req_dirty = 1; }
        }
        if (req_idx >= req_len) req_done = 1;
    }
    if (lit_key != 255 && lit_key != k) draw_key(lit_key, 0);
    draw_key(k, 1); lit_key = k; lit_timer = 6;
    spawn_spark(px + 4, 92 - k * 3 - up);
    landed_now = (k + 1) | (accent ? 0x80 : 0) | (up ? 0x20 : 0);
    // ---- musical tricks: the game notices what you play ----
    if (!attract && last_k != 255) {
        int8_t d = (int8_t)k - (int8_t)last_k;
        up_run = (d == 1) ? up_run + 1 : 0;
        dn_run = (d == -1) ? dn_run + 1 : 0;
        same_run = (d == 0 && b_len == 1) ? same_run + 1 : 0;
        trill_run = ((d == 1 || d == -1) && k == prev_k) ? trill_run + 1 : 0;
        if (up_run == 4 || dn_run == 4) { up_run = dn_run = 0; mys_last = k; start_sfx(6); do_trick("RUN!", 64, 1); }
        else if (accent && (d >= 2 || d <= -2)) { shake = 6; do_trick("LEAP!", 60, 2); }
        else if (trill_run == 3) { trill_run = 0; start_sfx(2); do_trick("TRILL!", 56, 3); }
        else if (same_run == 5) { same_run = 0; roll_steps = 4; do_trick("ROLL!", 60, 4); }
    }
    prev_k = last_k; last_k = k;
    (void)pos;
}

// ---------------- looper ----------------
uint8_t ev_x(uint8_t e) { return (e & 0x40) ? pad_x[(e & 0x0F) - 1] : 16 + ((e & 0x0F) - 1) * 16; }
uint8_t ev_h(uint8_t e) { return (e & 0x40) ? H_PAD : 0; }
void ghost_land(uint8_t pos) {
    uint8_t e = loop[pos];
    g_fx = ev_x(e); g_fh = ev_h(e);
    uint8_t d = 1;
    while (d < 32 && !loop[(pos + d) & 31]) d++;
    uint8_t ne = loop[(pos + d) & 31];
    g_tx = ev_x(ne); g_th = ev_h(ne);
    g_T = frames_of(step, d); g_t = 0;
    g_arc = d * 5 > 26 ? 26 : d * 5 + 3;
    if (g_th > g_fh) g_arc += 10;
}
void start_request(void) {
    req_len = 3 + venue;
    req[0] = rand() % NKEYS;
    for (uint8_t i = 1; i < req_len; i++) {
        int8_t d = (int8_t)(rand() % 5) - 2; int8_t n = (int8_t)req[i - 1] + d;
        if (n < 0) n = 1; if (n >= NKEYS) n = NKEYS - 2;
        req[i] = (uint8_t)n;
    }
    req_state = 1; req_show = 0; req_idx = 0; req_done = 0; req_dirty = 1; inst_set(0);
    show_banner("LISTEN!", 52, 4 * req_len + 4);
}
void loop_toggle(void) {                // SELECT: any loop playing -> stop them all; none -> record the last 2 bars
    if (loop_on || dloop_on || bloop_on) {
        loop_on = 0; dloop_on = 0; bloop_on = 0; NR32_REG = 0;
        show_banner("LOOP OFF", 0, 10); ch1_raw(1500, 0x00, 1, 0x71);
    } else capture();
    hud_dirty |= 16;                    // the L in the HUD
}
void capture(void) {
    uint8_t any = 0, i;
    for (i = 0; i < 32; i++) if (hist[i]) any = 1;
    if (!any) return;
    if (band_capture(hist)) { NR32_REG = 0; ch1_raw(1750, 0x00, 2, 0x62); start_sfx(1); return; }
    for (i = 0; i < 32; i++) loop[i] = hist[i];
    loop_on = 1; show_banner("LOOP!", 64, 16);
    { uint8_t n = 0; for (i = 0; i < 32; i++) if (loop[i]) n++;
      if (n >= 4 && loop_cool == 0) { add_score(10); loop_cool = 4; } }   // a real phrase, not spam
    NR32_REG = 0;
    ch1_raw(1750, 0x00, 2, 0x62); start_sfx(1);                     // record blip (two tones)
}
