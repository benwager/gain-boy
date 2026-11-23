// ============================================================
//  GAIN BOY - helpers in ROM bank 0 (bank 1 was full)
//  Jingles and queued effects, stage and crowd drawing,
//  per-gig reset, attract mode.
// ============================================================
#include "game.h"
#include "gig.h"
#include "band.h"

void sfx_step(uint8_t busy) {
    if (!sfx_kind) return;
    if (busy && sfx_kind != 1) return;                 // you just played a note: the jingle waits a 16th
    const uint8_t len[8] = {0, 2, 3, 4, 4, 4, 6, 6};
    uint8_t i = sfx_i, m = 0, env = 0, duty = 2, side = 0;
    switch (sfx_kind) {
        case 1: if (i == 1) ch1_raw(1850, 0x00, 2, 0x62); break;                            // record blip, 2nd tone
        case 2: m = key_note(5 + i) + 12; env = 0x71; duty = 0; side = (i & 1) ? 1 : 2; break; // sparkle up
        case 3: { const uint8_t iv[4] = {0, 4, 7, 12}; m = ROOT + 12 + iv[i]; env = 0x82; } break;   // song jingle
        case 4: { const uint8_t iv[4] = {0, 4, 7, 12}; m = ROOT + 12 + iv[i]; env = 0xC3; } break;   // fanfare
        case 5: if (!(i & 1)) { m = ROOT + (i ? 5 : 7); env = 0x74; } break;                 // "aww", slowly
        case 7: { int8_t r = chord_semi(); uint8_t th = chord_is_minor() ? 3 : 4;   // pad: a harp run up the new chord
                  const uint8_t up[6] = {0, 0, 7, 12, 12, 19}; uint8_t o = (i == 1) ? th : (i == 4) ? 12 + th : up[i];
                  m = key_note(0) + 12 + (uint8_t)((r + 12) % 12) + o; if (m > key_note(0) + 40) m -= 12;
                  env = 0x93; duty = 2; side = (i & 1) ? 1 : 2; } break;
        case 6: if (mys_last != 255) { m = key_note((mys_last + i) % NKEYS) + 12 * (1 + i / 3); env = 0x92; duty = 1; side = (i & 1) ? 1 : 2; } break;   // run
    }
    if (m) { uint16_t f = nfreq(m); pan2(side); NR21_REG = duty << 6; NR22_REG = env; NR23_REG = f & 0xFF; NR24_REG = 0x80 | (f >> 8); live_x_valid = 0; }
    if (++sfx_i >= len[sfx_kind]) sfx_kind = 0;
}

void play_q(uint8_t id) {
    switch (id) {
        case 1: ch1_raw(1300, 0x27, 2, 0x72); break;                              // jump whoosh
        case 2: ch1_raw(1700, 0x1D, 2, 0x62); break;                              // drop off a pad
        case 3: noise_hit(4, 0xC4, 0x01); ch1_raw(1900, 0x15, 0, 0xA2); break;    // bird: cymbal + chirp
        case 4: { uint16_t f = nfreq(ROOT + 24);                                  // the bell
                  pan2(0); NR21_REG = 0x00; NR22_REG = 0xF6; NR23_REG = f & 0xFF; NR24_REG = 0x80 | (f >> 8);
                  live_x_valid = 0; noise_hit(20, 0x62, 0x00); } break;
        case 5: noise_hit(6, 0xF4, 0x01); ch1_raw(1800, 0x19, 2, 0xF2); break;    // back from a BREAKDOWN: crash + kick
    }
}

void draw_stage(void) {
    uint8_t row[20], r, c;
    for (c = 0; c < 20; c++) row[c] = T_BLANK;
    for (r = 0; r < 18; r++) set_bkg_tiles(0, r, 20, 1, row);
    if (venue == 0) {                                   // GARAGE: a brick back wall
        for (c = 0; c < 20; c++) row[c] = T_BRICK;
        for (r = 1; r < 3; r++) set_bkg_tiles(0, r, 20, 1, row);
        for (c = 0; c < 20; c++) row[c] = T_BLANK;
    }
    for (uint8_t k = 0; k < NKEYS; k++) draw_key(k, 0);
    for (uint8_t p = 0; p < 4; p++) draw_pad(p, 0);
    set_bkg_tile_xy(0, 5, bounce_off ? T_SWBOFF : T_SWBON);       // BOUNCE switch (left wall)
    set_bkg_tile_xy(19, 5, T_SWKEY);                               // KEY UP box (right wall)
    uint8_t cl[2] = { T_CLOUDL, T_CLOUDR };
    if (venue == 2) { set_bkg_tiles(3, 3, 2, 1, cl); set_bkg_tiles(14, 5, 2, 1, cl); set_bkg_tiles(9, 2, 2, 1, cl); }
    venue_bgp = (venue == 1) ? 0xF9 : 0xE4;            // the CLUB is dim
    if (level == 5) { venue_bgp = 0x1B; OBP0_REG = 0x1B; }   // THE MOON: a night sky, light sprites
    hud_dirty = 15; glow_shown = 0xFF; want_lights = 255; want_speak = 0;
    BGP_REG = venue_bgp;
    crowd_frame = 255; lit_key = 255; lit_pad = 255;
    uint8_t lg[2] = { T_LEDGE, T_LEDGE };
    set_bkg_tiles(0, LEDGE_ROW, 2, 1, lg); set_bkg_tiles(18, LEDGE_ROW, 2, 1, lg);
    uint8_t sp[2] = { T_SPEAKER, T_SPEAKER };
    for (r = LEDGE_ROW + 1; r < 17; r++) { set_bkg_tiles(0, r, 2, 1, sp); set_bkg_tiles(18, r, 2, 1, sp); }
}

void reset_play(void) {                // called by gig.c at the start of every gig
    uint8_t i;
    song = (venue > 2) ? 0 : venue; fireworks = 0; shake = 0; energy = 0; cheered = 0;
    inst = 0; band_clear(); cur_wave = 255;
    tricks = 0; last_k = 255; prev_k = 255; clap_bars = 0; fast_bars = 0; tempo_mod = 0; sing_n = 0;
    sfx_kind = 0; q_sfx = 0;
    px = 72; height = 0; vis_h = 0; draw_h = 0; drop_through = 0; start_bounce(2);
    req_state = 0; req_cool = 3; req_dirty = 1;
    pad_seq = 0; power_steps = 0; pk_type = 0; pk_wait = 2; OBP0_REG = (level == 5) ? 0x1B : 0xE4;   // the moon: light sprites on a dark sky
    loop_on = 0; for (i = 0; i < 32; i++) hist[i] = 0; NR32_REG = 0; hide_sprites(SPR_GHOST, 4);
    mys_key = 255; mys_wait = 4; bird_on = 0; bad_on = 0; bad_wait = 3;
    hide_sprites(SPR_SPARK, 4); hide_sprites(SPR_BAD, 3); hide_sprites(SPR_PRESS, 8);
    move_sprite(SPR_BIRD, 0, 0); move_sprite(SPR_MYS, 0, 0); move_sprite(SPR_DISCO, 0, 0);
    for (i = 0; i < NECHO; i++) e_on[i] = 0;
    override_bars = 0; tw_life[0] = tw_life[1] = 0;
    for (i = 0; i < 16; i++) bar_notes[i] = prev_notes[i] = 255;
    riff_n = 0; sweet_bar = 0; last_sweet_k = 255; glow_shown = 0xFF;
    calc_glow(); want_glow = glow_mask;                 // markers from the very first bar
}

void pre_bar(void) {
    // RIFF! did you just play the same bar twice? (repetition is what makes a tune)
    uint8_t n = 0, same = 1;
    for (uint8_t i = 0; i < 16; i++) { if (bar_notes[i] != 255) n++; if (bar_notes[i] != prev_notes[i]) same = 0; }
    if (!attract && same && n >= 3) {
        if (riff_n < 3) {
            riff_n++;
            add_score(riff_n * 20);                         // 20, 40, 60: repetition makes a tune
            show_banner(riff_n == 1 ? "RIFF!" : riff_n == 2 ? "RIFF X2!" : "RIFF X3!", 0, 10);
            clap_bars = 2; energy += 6;                    // the crowd claps along with your riff
            spawn_star(40, 100); spawn_star(80, 96); spawn_star(120, 100);
        }
    } else riff_n = 0;
    for (uint8_t i = 0; i < 16; i++) { prev_notes[i] = bar_notes[i]; bar_notes[i] = 255; }
    sweet_bar = 0; last_sweet_k = 255;
    if (loop_cool) loop_cool--;
    // the chord for the coming bar
    uint8_t ob = override_bars; if (override_bars == 1) override_bars = 0;
    bar++; calc_glow(); bar--;
    override_bars = ob;
}

void draw_crowd(uint8_t f) {
    if (f == crowd_frame) return;
    crowd_frame = f;
    uint8_t row[20];
    uint8_t c3 = 0;
    for (uint8_t c = 0; c < 20; c++) {
        uint8_t a = (f >> ((c & 1) ? 4 : 0)) & 0x0F;   // two dancer groups, opposite phase
        uint8_t here = (venue == 2) || (venue == 1 && c3 != 2) || (venue == 0 && (c & 3) == 1);
        { const uint8_t leave[20] = {9,3,15,6,12,0,18,10,4,16,1,13,7,19,2,11,17,5,14,8};   // the order people walk out
          uint8_t m2 = (attract || jam) ? 100 : mood; if (leave[c] * 5 >= m2 + 4) here = 0; }   // a low mood thins the crowd
        if (++c3 == 3) c3 = 0;                         // every-third-column without a (slow) division
        row[c] = here ? T_CROWD1 + a : T_BLANK;
    }
    set_bkg_tiles(0, CROWD_ROW, 20, 1, row);
    if (attract) draw_version();       // keep the tiny mark over the crowd
}

void park_player(uint8_t hop) {          // results cards: centre stage, bobbing
    uint8_t base = hop ? T_P_JUMP : T_P_RUN1, py = BASE_Y - hop, x = 72;
    px = x; height = 0; vis_h = 0; draw_h = 0;
    for (uint8_t i = 0; i < 4; i++) set_sprite_tile(SPR_PLAYER + i, base + i);
    move_sprite(SPR_PLAYER + 0, x + 8,  py + 16); move_sprite(SPR_PLAYER + 1, x + 16, py + 16);
    move_sprite(SPR_PLAYER + 2, x + 8,  py + 24); move_sprite(SPR_PLAYER + 3, x + 16, py + 24);
}

void draw_version(void) {              // tiny version mark, bottom-right (title only)
    uint8_t buf[4], i;
    for (i = 0; i < 4; i++) buf[i] = letter_tile(VERSION_STR[i]);
    set_bkg_tiles(16, CROWD_ROW, 4, 1, buf);
}

void enter_attract(void) {
    arr = A_VERSE; countin = 0;
    attract = 1; venue = 2; kshift = 0; reset_play(); draw_stage();
    hide_sprites(SPR_TITLE, 9); hide_sprites(SPR_PRESS, 8); attract_shown = 255;
    uint8_t row[20]; for (uint8_t c = 0; c < 20; c++) row[c] = T_BLANK; set_bkg_tiles(0, 0, 20, 1, row);
    draw_version();
}
