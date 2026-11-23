// ============================================================
//  GAIN BOY - step sequencer (ROM bank 1)
//  do_step(): one 16th of the band + player landings.
// ============================================================
#pragma bank 1
#include "game.h"
#include "gig.h"
#include "band.h"


// ============================================================
//  sequencer (once per 16th)
// ============================================================
void do_step(uint8_t s) {
    const Song *v = &songs[song];
    uint8_t ds = s;
    uint16_t bit = 1u << ds;                              // (the band's patterns follow ds; you follow s)
    uint8_t pos = ((bar & 1) << 4) | s;
    int8_t ch = chord_semi();
    uint8_t melody_now = 0;
    landed_now = 0;

    if (s == 0 && override_bars) override_bars--;
    // the glow follows the chord; in the last beats of every 4 bars it points home instead
    if (inst == 1) want_glow = inst_mask((s + 2) & 14, 0);             // drums: the stars show the next hit
    else want_glow = ((bar & 3) == 3 && s >= 8) ? ((s & 2) ? 0x21 : 0x00) : glow_mask;
    if (roll_steps) { roll_steps--; ch1_raw(1150 + roll_steps * 60, 0x1C, 2, 0xA2); }

    // ---- drums ----
    uint8_t noise_used = 0;
    uint8_t fill = ((bar & 3) == 3) && s >= 12;               // last beat of every 4 bars
    uint8_t drums = !countin && arr != A_BREAK && inst != 1 && !dloop_on && !stut;   // the break drops the drums; so does you drumming
    if (stut && !(s & 3) && !countin) kick();          // BREAKDOWN: just a heartbeat kick under your notes
    uint8_t kick_now = drums && (v->kick & bit) != 0;
    if (countin && !(s & 3)) {                          // count-in: sticks on every beat, with the number
        noise_hit(56, 0x91, 0x01); noise_used = 1;
        const char * const nums[4] = { "1", "2", "3", "4" };
        show_banner(nums[s >> 2], 0, 3);
    }
    if (kick_now) { kick(); want_speak = 1; thump = 4; }
    if (q_sfx && !(kick_now && q_sfx != 4)) { play_q(q_sfx); q_sfx = 0; }   // queued sfx land on the grid
    if (!drums) { if (arr == A_BREAK && !(s & 3) && !noise_used) { noise_hit(60, 0x30, 0x00); noise_used = 1; } }   // break: just a soft tick
    else if (fill && s != 13 && arr != 0) {                   // drum fill
        if (v->snare) snare(); else noise_hit(52, 0x70 + ((s - 11) << 4), 0x24);
        noise_used = 1;
    }
    else if ((bar & 3) == 0 && s == 0 && bar) { crash(); noise_used = 1; }   // crash on the turnaround
    else if ((v->snare & bit) && arr != 0) { snare(); noise_used = 1; }          // the intro has no snare yet
    else if (v->ohat & bit) { ohat(); noise_used = 1; }
    else if (v->hat & bit) { noise_hit(59, (s & 4) ? 0x60 : 0x30, 0x00); noise_used = 1; }   // accented hats
    if (kick_now && !noise_used && v->kick_click) click();          // (only when the band's drums are playing)
    if (!noise_used && drums && A_BIG && (s & 3) == 2) { ohat(); noise_used = 1; }   // chorus: open hats push it
    if (!noise_used && drums) {
        if (clap_bars && (bit & 0x1410)) { noise_hit(48, 0xA1, 0x12); clap_frames = 3; noise_used = 1; }   // CLAP! clap . clap-clap
        else if (venue >= 1 && (bit & 0x2492)) { noise_hit(54, 0x41, 0x4B); noise_used = 1; }              // club: metal percussion
        else if (venue == 2 && (s & 1)) { noise_hit(60, 0x20, 0x00); noise_used = 1; }                     // big stage: 16th ghost hats
    }

    // ---- the crowd sings your tune back (wave channel) ----
    uint8_t singing = 0;
    if (sing_n && !loop_on && !(s & 1)) {
        if (sing_i < sing_n) { use_wave(2); wave_note(key_note(sing[sing_i++]) - 12, 256 - FPS * 10, 0x20); singing = 1; energy = 20; }
        else sing_n = 0;
    } else if (sing_n && !loop_on) singing = 1;
    if (loop_on) sing_n = 0;
    // ---- bass (unless your loop owns the wave channel) ----
    if (band_step((pos & 16) | ds, countin || stut, v->fps)) noise_used = 1;      // your beat loop and bass loop (silent in a BREAKDOWN)
    if (!loop_on && !singing && inst != 2 && !bloop_on && !stut) {
        use_wave(v->bass_wave);
        uint8_t root = ROOT - 24 + ch;
        if (countin) { }                                                  // no bass in the count-in
        else if (arr == A_BREAK) { if (bit & 0x0001) wave_note(root, 256 - v->fps * 15, 0x20); }   // break: long held roots
        else if (A_BIG && (bit & 0x0100)) wave_note(root + 12, 256 - v->fps * 6, 0x40);           // chorus: octave bounce
        else if (bit & 0x0001) wave_note(root, 256 - v->fps * 12, 0x20);
        else if (bit & 0x0040) wave_note(root + 12, 256 - v->fps * 6, 0x40);
        else if (bit & 0x0400) wave_note(root + 7, 256 - v->fps * 8, 0x20);
    }

    // ---- the bounce lands exactly on the grid ----
    if (pmode == M_BOUNCE) {
        uint8_t fall_on = 0;
        if (b_left == 1) {
            uint8_t kk; uint8_t hh = surface(cx(), b_from + b_H, &kk);
            if (kk == SURF_PAD && vis_h + 12 < H_PAD) hh = 0;
            if (vis_h > hh + 8) {                              // still high above the floor: fall one more 16th
                b_from = vis_h; b_t = 0; b_H = 0; tgt_s = hh; b_T = frames_of(step, 1);
                fall_on = 1;
            }
        }
        if (!fall_on && --b_left == 0) {
            land(pos); melody_now = (landed_now != 0);
            if (pmode == M_BOUNCE) {
                if (bounce_off || land_kind == SURF_PAD) { pmode = M_IDLE; b_H = 0; b_from = height; }   // pads: always stand still
                else start_bounce((keys & J_B) ? 1 : 2);
            }
        }
    } else if (pmode == M_IDLE) {
        if (down_cnt) down_cnt--;
        uint8_t rep = (inst == 1) ? (keys & (J_DOWN | J_B)) : (keys & J_B);   // drums repeat on DOWN too; notes sustain
        if (down_q || (rep && !down_cnt && hold_k == 255)) {
            uint8_t hold = inst != 1 && (down_q & 1) && (keys & J_DOWN) && height == 0;   // DOWN on the keys: hold the note
            down_q = 0; down_cnt = (keys & J_B) ? 1 : 2;
            b_from = height; b_H = 0; is_big = 0; dbl = 0; b_len = 2;
            sustain = hold; land(pos); sustain = 0; melody_now = (landed_now != 0);
            hold_k = hold ? (uint8_t)((cx() - 16) >> 4) : 255;
            if (pmode == M_BOUNCE) pmode = M_IDLE;
        }
    } else if (!(s & 1)) {
        if (pmode == M_SIT && !(keys & J_DOWN)) { start_bounce((keys & J_B) ? 1 : 2); }
        else if (pmode == M_WAIT) start_bounce((keys & J_B) ? 1 : 2);
        else if (pmode == M_STAND && big_queued) { big_queued = 0; start_bounce(4); q_sfx = 1; }
    }
    hist[pos] = landed_now;

    sfx_step(melody_now);
    // ---- your loop ----
    if (loop_on && loop[pos] && !stut) {
        uint8_t e = loop[pos];
        if (e & 0x40) {                                          // recorded chord change
            pad_chord = (e & 0x0F) - 1; override_bars = 4;
            ch = chord_semi();
            use_wave(v->loop_wave);
            wave_note(ROOT - 12 + ch, 256 - v->fps * 14, 0x20);
            if (!melody_now && !stab_frames) strum(pad_chord, 0x63);
        } else {
            use_wave(v->loop_wave);
            uint8_t m = key_note((e & 0x0F) - 1) - 12 + ((e & 0x80) ? 12 : 0) + ((e & 0x20) ? 12 : 0);
            wave_note(m, 256 - v->fps * 9, 0x20);
        }
        ghost_land(pos);
    }

    // ---- echoes fill the gaps ----
    for (uint8_t i = 0; i < NECHO; i++) {
        if (!e_on[i] || e_due[i] != tick) continue;
        if (!melody_now && !stab_frames && hold_k == 255) { lead(e_note[i], e_env[i], (e_env[i] == v->echo1) ? 1 : 2); live_x_valid = 0; }   // (not over a held note)
        e_on[i] = 0;
    }

    // ---- crowd: dances harder the more you play ----
    if (s == 0 || s == 8) { if (energy) energy -= (energy + 3) >> 2; }
    uint8_t lvl = energy < 3 ? 0 : energy < 8 ? 1 : energy < 15 ? 2 : 3;
    if (!(s & 1)) hype_tick();
    if (A_BIG && energy < 10) energy = 10;               // chorus: the whole crowd dances
    if (arr == A_BREAK && energy > 6) energy = 6;
    if (lvl == 3 && !cheered) { cheered = 1; NR41_REG = 0; NR42_REG = 0x3B; NR43_REG = 0x31; NR44_REG = 0xC0; }   // (with its own length: it always fades)   // crowd cheer: swelling noise, next drum cuts it
    if (lvl < 2) cheered = 0;
    uint8_t a = 0, b = 0;
    if (lvl == 1) { a = (s & 4) ? 1 : 0; b = a ^ 1; }
    else if (lvl == 2) { a = (s & 2) ? 1 : 0; b = a ^ 1; }
    else if (lvl == 3) { a = (s & 2) ? 2 : 1; b = (s & 2) ? 1 : 2; }
    if ((clap_bars && (bit & 0x1410)) || (sing_n && !(s & 1))) { a = 2; b = 2; }
    want_crowd = a | (b << 4);

    // ---- twinkles on keys your loop uses ----
    if (loop_on && !(s & 3)) {
        uint8_t e = loop[(pos + (rand() & 31)) & 31];
        if (e && !(e & 0x40)) for (uint8_t i = 0; i < 2; i++) if (!tw_life[i]) { tw_x[i] = ev_x(e) + 4 + (rand() & 7); tw_life[i] = 10; break; }
    }

    // ---- fuzzball comes and goes ----
    if (s == 0) {
        if (!bad_on && bad_wait && --bad_wait == 0) { bad_on = 1; bad_dir = rand() & 1; bad_x = bad_dir ? 16 : 136; ch1_raw(1950, 0x14, 0, 0x41); }
    }
    if (ban_t && --ban_t == 0) ban_dirty = 1;
    // ---- stage events: every 8 bars from the club up, the gig shifts for 4 bars ----
    if (s == 0 && !attract) {
        if (clap_bars) clap_bars--;
        if (fast_bars && --fast_bars == 0) tempo_mod = 0;
        if (venue >= 1 && (bar & 7) == 4 && req_state == 0 && !clap_bars && !fast_bars) {
            clap_bars = 4; if (!ban_t) show_banner("CLAP!", 60, 10);      // the crowd joins in (the tempo never changes)
        }
    }
    if (fireworks) {                                          // bursts of stars over the stage, on the beat
        fireworks--;
        if (!(s & 3)) { uint8_t fx = 24 + (rand() & 63) + (rand() & 31), fy = 20 + (rand() & 31);
            spawn_star(fx, fy); spawn_star(fx + 8, fy + 6); spawn_star(fx - 8, fy + 6); noise_hit(40, 0x71, 0x13); }
    }
    if (req_state == 2 && req_timer < 16) req_dirty = 1;      // running out of time: the notes blink

    // ---- crowd requests (Simon says) ----
    if (!attract) {
        // pickups appear on a ledge now and then
        if (s == 0) {
            if (!pk_type && pk_wait && --pk_wait == 0) {
                uint8_t r = rand() & 7;
                if (inst == 0 && r < 2 && !solo_used) pk_type = 3;          // DRUM SOLO: once per gig
                else if (inst == 0 && r < 4) pk_type = 4;                   // SYNTH for 4 bars
                else if (!fx_id && r < 7) pk_type = 5;                      // FX: the sound changes for 4 bars
                else pk_type = power_steps ? 0 : 2;                         // golden mic: star power
                if (!pk_type) pk_wait = 2;
                else { pk_side = rand() & 1; pk_life = 6; ch1_raw(1900, 0x00, 2, 0x41); }
            } else if (pk_type && pk_life && --pk_life == 0) { pk_type = 0; pk_wait = 2; move_sprite(SPR_PICK, 0, 0); }
        }
        if (req_state == 2) req_stall++;
        if (req_state == 1 && !(s & 3)) {                     // play the tune, one note per beat
            if (req_show < req_len) {
                uint8_t k = req[req_show++];
                if (!melody_now) lead(key_note(k), 0xA4, 0);
                draw_key(k, 1); if (lit_key != 255 && lit_key != k) draw_key(lit_key, 0);
                lit_key = k; lit_timer = 10; req_dirty = 1;
            } else {
                req_state = 2; req_idx = 0; req_timer = 64 + req_len * 8; req_dirty = 1; req_stall = 0;
                show_banner("PLAY IT!", 48, 10);
            }
        }
        if (req_state == 2) {
            if (req_done) {
                req_state = 0; req_cool = 4; req_dirty = 1; req_done = 0; pk_wait = 3;
                for (uint8_t i = 0; i < req_len; i++) sing[i] = req[i];
                sing_n = req_len; sing_i = 0;                 // the crowd sings your tune back
                award_star(40 + req_len * 10);
            } else if (--req_timer == 0) {
                req_state = 0; req_cool = 4; req_dirty = 1; pk_wait = 3;
                start_sfx(5);
            }
        }
    }

    if (power_steps) { power_steps--; OBP0_REG = (power_steps && (s & 1)) ? 0x93 : (level == 5 ? 0x1B : 0xE4); if (energy < 20) energy = 20; }   // (back to the stage's own sprite colours)

    // ---- creatures ----
    if (s == 0) {
        if (!bird_on && bird_wait && --bird_wait == 0) { bird_on = 1; bird_x = 168; bird_y = (rand() & 1) ? 46 : 62; bird_flee = 0; }
        if (mys_key == 255 && mys_wait && --mys_wait == 0) { mys_key = rand() % NKEYS; mys_bars = 3; }
        else if (mys_key != 255 && mys_bars && --mys_bars == 0) { mys_key = 255; mys_wait = 5 + (rand() & 3); move_sprite(SPR_MYS, 0, 0); }
    }
    // ---- venue lights ----
    if (venue == 2 && !(s & 1)) {                        // BIG STAGE: chasing stage lights
        want_lights = s;
    }
}

