// ============================================================
//  GAIN BOY v1.0  -  boombox bouncing on a piano
//  Ben Wager                         build: GBDK-2020 4.5.0
// ============================================================
//  Where you land is what plays, always on the beat.
//  Nothing sounds wrong; the tempo never changes.
//
//  Title: A = Tour, START = Jam.
//  Bank 1 is split across main/songs/sound/player/seq (.c files).
// ============================================================
#pragma bank 1
#include "game.h"
#include "gig.h"
#include "band.h"
#include "save.h"

// ================= fast maths: 64-step parabola tables (no 32-bit maths per frame) =================
uint8_t para64[65];   // 4p(64-p)/64^2 * 256  : the arc, 0..256
uint16_t sq64[65];    // (p/64)^2 * 256       : ease-in weight, 0..256
void init_tables(void) {
    for (uint8_t p = 0; p <= 64; p++) {
        uint16_t a = ((uint16_t)p * (64 - p)) >> 2; para64[p] = a > 255 ? 255 : (uint8_t)a;
        sq64[p] = ((uint16_t)p * p) >> 4;
    }
}
uint8_t phase64(uint8_t t, uint8_t T) { if (!T) return 64; uint16_t p = ((uint16_t)t << 6) / T; return p > 64 ? 64 : (uint8_t)p; }

// ================= state =================
int8_t tempo_mod = 0;                    // FASTER! event
int8_t kshift = 0;                       // secret key change (+1 semitone each time)
uint8_t song = 0;
uint8_t step = 0, bar = 0, tick = 0, frame_in_step = 0, step_len = 8;
uint8_t keys, prev_keys, pressed;
uint16_t idle = 0;
uint8_t attract = 1;

// player
uint8_t px = 64, pmode = M_BOUNCE, height = 0;
uint8_t b_from = 0, b_H = 12, b_T = 16, b_t = 0, b_left = 2, b_len = 2, big_queued = 0;
uint8_t is_big = 0, dbl = 0;          // big bounce in progress / double jump used (pads reachable)
uint16_t loops = 0;                      // main-loop iterations (used by the timing tests)
// visuals requested by the sequencer, drawn on the quiet frames between 16ths (sound stays on the beat)
uint8_t want_glow = 0, want_crowd = 0, want_lights = 255, want_speak = 0;
uint8_t sfx_frames = 0, sfx_kind = 0, sfx_i = 0, q_sfx = 0, cheered = 0, strum_dir = 0, roll_steps = 0;
uint8_t lit_key = 255, lit_timer = 0, lit_pad = 255, lit_pad_timer = 0;

// harmony
uint8_t override_bars = 0, pad_chord = 0;

// looper
uint8_t hist[32], loop[32], loop_on = 0, landed_now = 0, banner = 0, banner_vibe = 0;
// ghost
uint8_t g_x = 64, g_fx = 64, g_tx = 64, g_fh = 0, g_th = 0, g_t = 0, g_T = 0, g_arc = 0, g_y = 0;

// fx
uint8_t e_on[NECHO], e_due[NECHO], e_note[NECHO], e_env[NECHO];
uint8_t stab_frames = 0; uint16_t stab_x[3];
uint8_t clap_frames = 0;
uint8_t live_x_valid = 0; uint16_t live_x = 0; uint8_t live_frames = 0;

// charm
uint8_t energy = 0, crowd_frame = 255;
uint8_t bad_on = 0, bad_x = 0, bad_dir = 1, bad_squash = 0, bad_wait = 4, bad_t = 0;
uint8_t tw_x[2], tw_life[2];

uint8_t spk_x[NSPARK], spk_y[NSPARK], spk_life[NSPARK];
uint8_t ai_dir = 0, ai_t = 0;
uint8_t attract_shown = 255;
// banners
const char *ban_text = 0; uint8_t ban_t = 0, ban_x = 60, ban_dirty = 0;
// game: requests, stars, venue
uint8_t req_state = 0, req_len = 3, req[6], req_idx = 0, req_show = 0, req_cool = 3, req_dirty = 1;
uint16_t req_timer = 0;
uint8_t req_done = 0, req_stall = 0;
// pickups on the ledges: 1 = request bubble, 2 = golden mic
uint8_t pk_type = 0, pk_side = 0, pk_wait = 2, pk_life = 0;
uint8_t power_steps = 0;                 // STAR POWER from the golden mic
uint8_t pad_seq = 0;                     // secret: all four pads left to right = key change
uint8_t drop_through = 0, down_lock = 0; // DOWN on a pad drops you to the floor
uint8_t bounce_off = 0;                  // the BOUNCE switch (left wall): 1 = stand still, DOWN plays the key
uint8_t ghost_vis = 0, dbl_q = 0;
uint8_t land_kind = 0, down_q = 0, down_cnt = 0, hold_k = 255, sustain = 0;
uint8_t stut = 0, stut_s0 = 0, stut_prev = 0;   // BREAKDOWN: hold DOWN while bouncing and the band drops out
uint8_t sw_cool = 0;
uint8_t tgt_s = 0;                       // smoothed landing height (no snapping mid-jump)
uint8_t vis_h = 0;
uint8_t draw_h = 0;                      // what's on screen: follows height, max 6px per frame
uint8_t venue = 2, venue_bgp = 0xE4;
uint8_t thump = 0, shake = 0;            // speakers pump on the kick; screen shake on big moments
uint8_t fireworks = 0;                   // steps of ENCORE fireworks left
// musical tricks
uint8_t last_k = 255, prev_k = 255, up_run = 0, dn_run = 0, same_run = 0, trill_run = 0, tricks = 0;
// stage events
uint8_t clap_bars = 0, fast_bars = 0;
// the crowd sings your tune back
uint8_t sing_n = 0, sing_i = 0, sing[6];
// the bell
uint8_t bell_cool = 0, from_pad = 0;      // from_pad: this jump started on a chord pad
// creatures
uint8_t bird_on = 0, bird_y = 46, bird_flee = 0, bird_wait = 6, bird_t = 0;
int16_t bird_x = 0;
uint8_t mys_key = 255, mys_bars = 0, mys_wait = 4, mys_last = 255;
const char * const venue_names[3] = { "GARAGE", "CLUB!", "BIG STAGE" };

// ---- musical scoring ----
uint8_t glow_mask = 0, glow_shown = 0xFF;   // keys that fit the current chord
uint8_t sweet_bar = 0, last_sweet_k = 255;  // sweet notes scored this bar (capped)
uint8_t bar_notes[16], prev_notes[16];      // this bar's phrase / last bar's, for RIFF!
uint8_t riff_n = 0;
uint8_t loop_cool = 0;                     // bars until a new loop can score again
uint8_t paused = 0;


// ============================================================
//  stage
// ============================================================
void draw_key(uint8_t k, uint8_t state) {   // 0 normal, 1 pressed, 2 ghost
    uint8_t base;
    if (state == 1) base = T_KEYP;
    else if (state == 2) base = (k & 1) ? T_KEYA : T_KEYB;
    else base = (k & 1) ? T_KEYB : T_KEYA;
    uint8_t t[4] = { base, base + 1, base + 2, base + 3 };
    if (state == 0 && glow_shown != 0xFF && ((glow_shown >> k) & 1)) {   // a star on the key
        uint8_t st = (k & 1) ? T_KEYBS : T_KEYAS; t[0] = st; t[1] = st + 1;
    }
    set_bkg_tiles(2 + k * 2, KEY_ROW, 2, 2, t);
}
void draw_pad(uint8_t p, uint8_t lit) {
    uint8_t t[3];
    if (lit) { t[0] = t[1] = T_PADLIT; }
    else { t[0] = T_PADS1 + p; t[1] = T_PADR; }
    set_bkg_tiles(pad_x[p] >> 3, PAD_ROW, 2, 1, t);
}
void show_banner(const char *t, uint8_t x, uint8_t steps) {
    uint8_t n = 0; while (t[n]) n++;
    ban_text = t; ban_x = 80 - n * 4 - 8 + 8; ban_t = steps; ban_dirty = 1;   // always centred
    (void)x;
}
void draw_speakers(uint8_t t) {
    uint8_t sp[4] = { t, t, t, t };
    set_bkg_tiles(0, LEDGE_ROW + 1, 2, 2, sp); set_bkg_tiles(18, LEDGE_ROW + 1, 2, 2, sp);
}
uint8_t letter_tile(char c) {
    const char *s = LETTER_STR;
    for (uint8_t i = 0; s[i]; i++) if (s[i] == c) return T_LETTERS + i;
    return T_BLANK;
}
void show_text(uint8_t spr, const char *s, uint8_t x, uint8_t y, uint8_t slots) {
    uint8_t i = 0;
    for (; s[i] && i < slots; i++) {
        if (s[i] == ' ') { move_sprite(spr + i, 0, 0); continue; }
        set_sprite_tile(spr + i, letter_tile(s[i]));
        move_sprite(spr + i, x + i * 8 + 8, y + 16);
    }
    for (; i < slots; i++) move_sprite(spr + i, 0, 0);
}
void hide_sprites(uint8_t from, uint8_t n) { for (uint8_t i = 0; i < n; i++) move_sprite(from + i, 0, 0); }
void spawn_tile(uint8_t x, uint8_t y, uint8_t t) {
    for (uint8_t i = 0; i < NSPARK; i++) if (!spk_life[i]) { spk_x[i] = x; spk_y[i] = y; spk_life[i] = 26; set_sprite_tile(SPR_SPARK + i, t); return; }
}
void spawn_spark(uint8_t x, uint8_t y) { spawn_tile(x, y, T_NOTE); }
void spawn_star(uint8_t x, uint8_t y)  { spawn_tile(x, y, T_STAR); }

void main(void) {
    uint8_t i;
    DISPLAY_OFF;
    set_bkg_data(0, N_TILES, tiles);
    set_sprite_data(0, N_TILES, tiles);
    BGP_REG = 0xE4; OBP0_REG = 0xE4; OBP1_REG = 0x90;
    venue = 2; draw_stage();
    for (i = 0; i < 40; i++) move_sprite(i, 0, 0);
    for (i = 0; i < NSPARK; i++) set_sprite_tile(SPR_SPARK + i, T_NOTE);
    for (i = 0; i < 4; i++) set_sprite_prop(SPR_GHOST + i, S_PALETTE);
    set_sprite_tile(SPR_TW, T_STAR); set_sprite_tile(SPR_TW + 1, T_STAR);
    NR52_REG = 0x80; NR50_REG = 0x77; NR51_REG = 0xFF;
    initrand(0x5151);
    save_load();                               // high score, unlocked songs... from the battery save
    init_tables();
    SHOW_BKG; SHOW_SPRITES; HIDE_WIN; DISPLAY_ON;
    step = 15; frame_in_step = 7;
    start_bounce(2);

    while (1) {
        wait_vbl_done();
        loops++;
        prev_keys = keys; keys = joypad(); pressed = keys & ~prev_keys;
        const Song *v = &songs[song];

        if (keys) {
            idle = 0;
            if (attract) {                        // a new player: a fresh run from the garage
                attract = 0; hide_sprites(SPR_TITLE, 17); initrand(DIV_REG); attract_shown = 255;
                if (pressed & J_START) gig_jam(); else gig_new_run();   // START = JAM (free play), anything else = the TOUR
                prev_keys = keys; continue;
            }
        } else if (++idle > (jam ? 6000 : 1500) && !attract && !gstate) enter_attract();   // jammers get longer to listen

        // ---------- between gigs: the results card ----------
        // ---------- PAUSE (START): SELECT quits to the title ----------
        if (paused) {                              // the pause menu (gig.c)
            uint8_t r = pause_frame(pressed);
            if (r) {
                paused = 0; NR50_REG = 0x77; ban_dirty = 1; req_dirty = 1;
                if (r == 2) enter_attract(); else draw_stage();   // put the stage back
            }
            continue;
        }
        if ((pressed & J_START) && !attract && !gstate) {
            paused = 1; NR50_REG = 0x00;
            hide_sprites(SPR_TITLE, 9); hide_sprites(SPR_PRESS, 8);
            pause_open();
            continue;
        }
        if (gstate) {
            if (gig_frame(pressed) == 2) enter_attract();
            hud_draw();
            continue;
        }

        if (attract) {   // demo: wander, visit the pads now and then, rest to loop
            if (attract_shown != (bar & 3)) {          // the title cycles: TOUR, JAM, high score
                attract_shown = bar & 3;
                show_text(SPR_TITLE, "GAIN BOY", 48, 20, 9);
                if (attract_shown == 1) show_text(SPR_PRESS, "ST = JAM", 48, 36, 8);
                else if (attract_shown == 2 && best_score) { char b[9]; best_text(b); show_text(SPR_PRESS, b, 48, 36, 8); }
                else show_text(SPR_PRESS, "A = TOUR", 48, 36, 8);
                draw_version();
            }
            if (ai_t == 0) { ai_dir = rand() % 3; ai_t = 16 + (rand() & 31); }
            ai_t--;
            keys = ai_dir == 1 ? J_LEFT : ai_dir == 2 ? J_RIGHT : 0;
            pressed = ((ai_t & 7) == 0 && (rand() & 3) == 0) ? J_A : 0;   // jumps, sometimes doubles
        }

        if ((pressed & J_A) && pmode == M_IDLE) {          // standing: jump right now
            pmode = M_BOUNCE; b_from = height; b_t = 0; b_left = 1; b_H = 0; tgt_s = height;
            is_big = 0; dbl = 0; b_T = f2b();
        }
        if ((pressed & J_DOWN) && pmode == M_IDLE) down_q |= 1;        // standing: DOWN plays (and holds) the key
        if ((pressed & J_B) && pmode == M_IDLE) down_q |= 2;           // B plays it too (hold B = 16ths)
        if (hold_k != 255) {                                            // a held note: release, or glide to a new key
            if (!(keys & J_DOWN) || pmode != M_IDLE) { NR22_REG = 0; hold_k = 255; live_x_valid = 0; }
            else if (cx() >= 16 && cx() < 144) {
                uint8_t k2 = (cx() - 16) >> 4;
                if (k2 != hold_k) {
                    uint16_t f = nfreq(key_note(k2) + ((keys & J_UP) ? 12 : 0));
                    NR23_REG = f & 0xFF; NR24_REG = (f >> 8) & 7; live_x = f; live_frames = 0;
                    if (lit_key != 255) draw_key(lit_key, 0);
                    draw_key(k2, 1); lit_key = k2; lit_timer = 6; hold_k = k2;
                }
                lit_timer = 6;                                          // the key stays lit while you hold it
            }
        }
        stut = (keys & J_DOWN) && pmode == M_BOUNCE && b_from < H_PAD && !attract && !gstate;
        if (stut && !stut_prev && !ban_t) show_banner("BREAKDOWN", 0, 10);
        if (stut_prev && !stut) { q_sfx = 5; shake = 10; energy += 10; }   // let go: the band slams back in with a crash
        stut_prev = stut;
        if ((pressed & J_A) && pmode == M_BOUNCE) {
            if (b_from >= H_PAD && !is_big) from_pad = 1;    // jumping off a pad: the bell is in reach
            if (!is_big) {                                   // jump: becomes a big bounce right now
                is_big = 1; extend_bounce(2, 28);
                if (height < H_PAD - 12) {                       // from below: a jump never reaches the pads
                    if (height + b_H > H_PAD - 12) b_H = H_PAD - 12 - height;
                } else if (b_H > 30) b_H = 30;                   // from a pad: a hop you can steer to the next pad
                q_sfx = 1;                                   // whoosh up (on the next 16th)
            } else if (!dbl) {                               // double jump: only way up to the pads
                dbl = 1; dbl_jump();
                start_sfx(2);                // sparkle chime
            }
        } else if ((pressed & J_A) && pmode == M_STAND) { if (big_queued) dbl_q = 1; big_queued = 1; }   // a 2nd tap while the jump waits for the beat: buffered
        if ((pressed & J_DOWN) && pmode == M_BOUNCE && b_from >= H_PAD) {   // on a pad: DOWN = drop to the floor
            drop_through = 1; down_lock = 1; is_big = 0; dbl = 0;
            b_from = height; b_t = 0; b_left = 2; b_H = 4; tgt_s = height;
            b_T = f2b() + frames_of((step + 1) & 15, 1);
            q_sfx = 2;
        }
        if (!(keys & J_DOWN)) down_lock = 0;

        if (pressed & J_SELECT) loop_toggle();          // SELECT: loop on/off for what you're playing (songs: pause menu)
        {   // walk 1px a frame; in a jump you move 1.5px a frame (air control), enough to hop pad to pad
            uint8_t sp = (pmode == M_BOUNCE && (is_big || dbl)) ? 2 : ((loops & 1) ? 2 : 1);   // walk 1.5px/frame, fly 2px/frame
            if ((keys & J_LEFT) && px > 0) px = (px > sp) ? px - sp : 0;
            if ((keys & J_RIGHT) && px < 144) px = (px + sp < 144) ? px + sp : 144;
        }

        // ---- clock (with swing) ----
        if (++frame_in_step >= step_len) {
            frame_in_step = 0; tick++;
            step = (step + 1) & 15; if (step == 0) { bar++; if (!attract && !gstate) gig_bar_tick(); }
            step_len = (step & 1) ? FPS - v->swing : FPS + v->swing;
            { uint8_t want = stut ? (venue_bgp == 0xF9 ? 0xFE : 0xF9) : venue_bgp; if (BGP_REG != want) BGP_REG = want; }   // the stage dims in a BREAKDOWN
            if (gstate) continue;                    // the gig just ended this bar
            do_step(step);
        }
        if (clap_frames && --clap_frames == 1) noise_hit(46, 0x81, 0x22);
        if (thump && --thump == 0 && frame_in_step) draw_speakers(T_SPEAKER);
        if (shake) { shake--; SCY_REG = (shake & 2) ? 1 : 0; SCX_REG = (shake & 1) ? 1 : 0; if (!shake) { SCY_REG = 0; SCX_REG = 0; } }

        // ---- movement between surfaces ----
        if (dbl_q && pmode == M_BOUNCE && is_big && b_t >= 4) { dbl_q = 0; if (!dbl) { dbl = 1; dbl_jump(); start_sfx(2); } }   // the buffered double jump
        if (pmode != M_BOUNCE && pmode != M_STAND) dbl_q = 0;
        if (pmode == M_BOUNCE) {
            uint8_t kind, peak = b_from + b_H;
            uint8_t tgt = surface(cx(), peak, &kind);
            if (kind == SURF_PAD && height + 6 < H_PAD && b_t * 2 > b_T) tgt = 0;   // coming down below the pad: not landable
            if (b_from + 4 >= H_PAD && tgt < H_PAD && cx() >= 24 && cx() < 136) tgt = H_PAD;   // hopping between pads: hold pad level over the gaps
            if (b_t < b_T) b_t++;
            uint8_t p = phase64(b_t, b_T);
            int16_t arc = ((int16_t)b_H * para64[p]) >> 8;
            if (tgt_s < tgt) tgt_s = (tgt - tgt_s > 4) ? tgt_s + 4 : tgt;           // glide, never snap
            else if (tgt_s > tgt) tgt_s = (tgt_s - tgt > 4) ? tgt_s - 4 : tgt;
            int16_t lift = (((int16_t)tgt_s - (int16_t)b_from) * (int16_t)sq64[p]) >> 8;
            int16_t y = (int16_t)b_from + lift + arc;
            height = y < 0 ? 0 : (uint8_t)y;
            vis_h = height;
        } else if (pmode == M_STAND) {
            if (cx() >= 16 && cx() < 144) {               // stepped off the ledge: land on the key on the next step
                pmode = M_BOUNCE; is_big = 0; dbl = 0;
                b_from = height; b_t = 0; b_left = 1; b_H = 3; tgt_s = height;
                b_T = f2b();
            }
            // collect a pickup waiting on this ledge
            if (pk_type && ((cx() < 16) ? 0 : 1) == pk_side && !attract) {
                if (pk_type == 1) start_request();
                else if (pk_type == 5) { fx_power(); start_sfx(2); energy = 25; }
                else if (pk_type >= 3) { inst_power(pk_type - 2); start_sfx(2); energy = 25; glow_shown = 0xFE; }
                else { power_steps = 64; show_banner("POWER UP!", 44, 16); start_sfx(2); energy = 25; }
                pk_type = 0; pk_wait = 2 + (rand() & 1); move_sprite(SPR_PICK, 0, 0);
                spawn_star(pk_side ? 148 : 4, 96);
            }
        }
        if (pmode == M_IDLE && height == 0) fuzz_touch();
        if (pmode == M_IDLE) {
            if (height >= H_PAD && pad_under(cx()) < 0) {    // walked off the pad: fall to the keys
                pmode = M_BOUNCE; drop_through = 1; b_from = height; b_t = 0; b_left = 1; b_H = 0; tgt_s = height;
                is_big = 0; dbl = 0; b_T = f2b();
            } else if (height < H_PAD && (cx() < 16 || cx() >= 144)) { height = H_LEDGE; pmode = M_STAND; }
        }
        // ---- the wall switches (double jump to reach): BOUNCE on the left, LOOP on the right ----
        if (sw_cool) sw_cool--;
        else if (!attract && dbl && height > 48 && (cx() < 16 || cx() >= 144)) {   // only a real double jump reaches them
            sw_cool = 60; shake = 4; ch1_raw(1700, 0x00, 1, 0x81);
            if (cx() < 16) {
                bounce_off ^= 1; set_bkg_tile_xy(0, 5, bounce_off ? T_SWBOFF : T_SWBON);
                show_banner(bounce_off ? "STAND" : "BOUNCE!", 0, 12);
                if (!bounce_off && pmode == M_IDLE) pmode = M_WAIT;   // start bouncing on the next 8th
            } else {                                     // KEY UP box: the whole song modulates up a step
                key_up ^= 1;                              // KEY UP box: lifts the song a key; hit it again to bring it back
                if (key_up) { kshift += 2; show_banner("KEY UP!", 0, 14); start_sfx(4); energy = 25; add_score(10); }
                else { kshift -= 2; show_banner("KEY DOWN", 0, 12); start_sfx(5); }
                BGP_REG = 0xE5;
            }
        }
        if (pmode == M_WAIT && height) height = height > 2 ? height - 2 : 0;
        if (pmode == M_SIT) { /* stays where it sat */ }

        // ---- per-frame sound ----
        if (stab_frames) {
            uint8_t idx;
            switch (strum_dir) {
                case 0: idx = mod3[stab_frames & 31]; break;                  // fast up
                case 1: idx = 2 - mod3[stab_frames & 31]; break;              // fast down
                case 2: idx = (stab_frames > 12) ? 0 : (stab_frames > 6 ? 1 : 2); break;   // rake
                default: idx = mod3[(stab_frames >> 1) & 31]; break;          // slow roll
            }
            uint16_t f = stab_x[idx]; NR23_REG = f & 0xFF; NR24_REG = (f >> 8) & 7; stab_frames--;
        }
        else if (live_x_valid && (v->vib || fx_id == 3)) {
            if (live_frames < 255) live_frames++;
            if (live_frames > 10) {
                const int8_t vib[8] = {0, 1, 2, 1, 0, -1, -2, -1};
                int16_t f = (int16_t)live_x + vib[(live_frames >> 1) & 7] * (fx_id == 3 ? 5 : v->vib);   // WOBBLE: deep vibrato
                NR23_REG = f & 0xFF; NR24_REG = (f >> 8) & 7;
            }
        }

        // ---- lights ----
        if (lit_timer && --lit_timer == 0 && lit_key != 255) { draw_key(lit_key, 0); lit_key = 255; }
        if (lit_pad_timer && --lit_pad_timer == 0 && lit_pad != 255) { draw_pad(lit_pad, 0); lit_pad = 255; }

        // ---- ghost (only while the melody loop plays; hidden the moment it stops, however it stops) ----
        if (!loop_on && ghost_vis) { hide_sprites(SPR_GHOST, 4); ghost_vis = 0; }
        if (loop_on) {
            ghost_vis = 1;
            if (g_t < g_T) g_t++;
            uint8_t gp = phase64(g_t, g_T);
            g_x = (uint8_t)((int16_t)g_fx + ((((int16_t)g_tx - (int16_t)g_fx) * (int16_t)gp) >> 6));
            int16_t base = (int16_t)g_fh + ((((int16_t)g_th - (int16_t)g_fh) * (int16_t)gp) >> 6);
            g_y = (uint8_t)(base + (((int16_t)g_arc * para64[gp]) >> 8));
            uint8_t gb = (g_y > g_fh + 3 && g_y > g_th + 3) ? T_P_JUMP : T_P_RUN1;
            uint8_t gy = BASE_Y - g_y;
            for (i = 0; i < 4; i++) set_sprite_tile(SPR_GHOST + i, gb + i);
            move_sprite(SPR_GHOST + 0, g_x + 8,  gy + 16); move_sprite(SPR_GHOST + 1, g_x + 16, gy + 16);
            move_sprite(SPR_GHOST + 2, g_x + 8,  gy + 24); move_sprite(SPR_GHOST + 3, g_x + 16, gy + 24);
        }

        // ---- fuzzball ----
        if (bad_on) {
            bad_t++;
            if (bad_squash) {
                set_sprite_tile(SPR_BAD, T_SQUASH);
                if (--bad_squash == 0) { bad_on = 0; bad_wait = 4 + (rand() & 3); move_sprite(SPR_BAD, 0, 0); }
            } else {
                if ((bad_t & 1) || level >= 3) { if (bad_dir) { if (++bad_x >= 136) bad_dir = 0; } else { if (--bad_x <= 16) bad_dir = 1; } }   // twice as fast from the ARENA on
                set_sprite_tile(SPR_BAD, (step & 2) ? T_FUZZ1 : T_FUZZ2);
            }
            if (bad_on) move_sprite(SPR_BAD, bad_x + 8, 112 + 16 - ((step & 2) && !bad_squash ? 2 : 0));
        }

        // ---- sparks + twinkles ----
        for (i = 0; i < NSPARK; i++) if (spk_life[i]) {
            spk_life[i]--; if (spk_life[i] & 1) spk_y[i]--;
            if (spk_life[i]) move_sprite(SPR_SPARK + i, spk_x[i] + 8 + ((spk_life[i] >> 2) & 1), spk_y[i] + 16);
            else move_sprite(SPR_SPARK + i, 0, 0);
        }
        for (i = 0; i < 2; i++) if (tw_life[i]) {
            tw_life[i]--;
            if (tw_life[i] && (tw_life[i] & 2)) move_sprite(SPR_TW + i, tw_x[i] + 8, 116 + 16); else move_sprite(SPR_TW + i, 0, 0);
        }

        // ---- text ----
        // ---- bird: bounce into it for a cymbal ----
        if (bird_on) {
            bird_t++;
            if (bird_flee) { bird_x -= 2; bird_y -= 2; } else if (bird_t & 1) bird_x--;
            if (bird_x < -8 || bird_y < 4) { bird_on = 0; bird_wait = 5 + (rand() & 3); move_sprite(SPR_BIRD, 0, 0); }
            else {
                uint8_t pty = BASE_Y - height;
                if (!bird_flee && !attract && bird_x + 8 > px && bird_x < px + 16 && bird_y + 8 > pty && bird_y < pty + 16) {
                    bird_flee = 1; q_sfx = 3; add_score(8);
                    spawn_spark((uint8_t)bird_x, bird_y); energy += 4;
                }
                set_sprite_tile(SPR_BIRD, (bird_t & 8) ? T_BAT1 : T_BAT2);
                move_sprite(SPR_BIRD, (uint8_t)(bird_x + 8), bird_y + 16);
            }
        }
        // ---- the bell: jump from a pad to ring it ----
        if (bell_cool) bell_cool--;
        {
            uint8_t pty = BASE_Y - height;
            if (!bell_cool && !attract && from_pad && pmode == M_BOUNCE && px + 16 > BELL_X - 4 && px < BELL_X + 12 && pty < BELL_Y + 10) {
                bell_cool = 90; shake = 8; energy = 25;
                uint16_t f = nfreq(ROOT + 24);
                (void)f; q_sfx = 4;
                spawn_star(BELL_X - 10, BELL_Y + 8); spawn_star(BELL_X + 10, BELL_Y + 8); spawn_star(BELL_X, BELL_Y + 14);
                do_trick("DING!", 60, 5);
            }
            set_sprite_tile(SPR_BELL, T_BELL);
            move_sprite(SPR_BELL, BELL_X + 8 + ((bell_cool > 70) ? ((bell_cool & 4) ? 2 : -2) : 0), BELL_Y + 16);
        }
        // ---- ? key ----
        if (mys_key != 255) { set_sprite_tile(SPR_MYS, T_QBLK); move_sprite(SPR_MYS, 16 + mys_key * 16 + 4 + 8, 108 + 16 - ((tick & 2) ? 1 : 0)); }
        // ---- pickup on a ledge ----
        if (!attract) {
            if (pk_type) {
                set_sprite_tile(SPR_PICK, pk_type == 1 ? T_BUBBLE : pk_type == 3 ? T_DRUMPK : pk_type == 4 ? T_BASSPK : pk_type == 5 ? T_FXPK : T_MIC);
                move_sprite(SPR_PICK, (pk_side ? 148 : 4) + 8, 100 + 16 - ((tick & 4) ? 2 : 0));
            } else move_sprite(SPR_PICK, 0, 0);
            // ---- hint arrow if you're stuck on a request ----
            if (req_state == 2 && req_stall > 32 && req_idx < req_len && (tick & 2)) {
                set_sprite_tile(SPR_HINT, T_ARROW); move_sprite(SPR_HINT, 16 + req[req_idx] * 16 + 4 + 8, 100 + 16);
            } else move_sprite(SPR_HINT, 0, 0);
        }
        // ---- disco ball in the CLUB ----
        if (venue == 1) { set_sprite_tile(SPR_DISCO, (step & 2) ? T_STAR : T_SKYDOT); move_sprite(SPR_DISCO, 76 + 8, 6 + 16); }
        else move_sprite(SPR_DISCO, 0, 0);

        if (!attract) {
            if (ban_dirty && frame_in_step == 6 && step != 15) {   // a quiet frame (never on a beat, never with pre_bar)
                ban_dirty = 0;
                banner_bg((ban_t && ban_text) ? ban_text : 0);   // drawn into the background: never cut off
            }
            if (req_dirty) {                                   // the request: notes turn into stars
                req_dirty = 0;
                uint8_t x0 = 80 - req_len * 5;
                for (i = 0; i < 5; i++) {
                    uint8_t show = 0, t = T_NOTE;
                    if (req_state == 1 && i < req_show) show = 1;
                    if (req_state == 2 && i < req_len) { show = !(req_timer < 16 && (step & 1)); t = (i < req_idx) ? T_STAR : T_NOTE; }
                    if (show) { set_sprite_tile(SPR_PRESS + i, t); move_sprite(SPR_PRESS + i, x0 + i * 10 + 8, 20 + 16); }
                    else move_sprite(SPR_PRESS + i, 0, 0);
                }
            }
            if (step == 15 && frame_in_step == 6) pre_bar();
            // ---- deferred drawing: one job per quiet frame, never on the frame a 16th starts ----
            if (frame_in_step == 1 && want_speak) { want_speak = 0; draw_speakers(T_SPEAKER2); }
            else if (frame_in_step == 2) draw_crowd(want_crowd);
            else if (frame_in_step == 3) draw_glow(want_glow);
            else if (frame_in_step == 4 && want_lights != 255) {
                uint8_t row[20], ls = want_lights; want_lights = 255;
                for (uint8_t c = 0; c < 20; c++) row[c] = (((c >> 1) + (ls >> 1)) & 3) == 0 ? T_LIGHTON : T_LIGHTOFF;
                set_bkg_tiles(0, 1, 20, 1, row);
            }
            else if (frame_in_step == 5 && hud_dirty) hud_draw();
        }

        // ---- player ----
        uint8_t pb = (pmode == M_SIT) ? T_P_CROUCH :
                     (pmode == M_BOUNCE && height > b_from + 3 && b_t > 1) ? T_P_JUMP : T_P_RUN1;
        if (pmode == M_BOUNCE && is_big) pb = T_P_JUMP;
        if (draw_h < height) draw_h = (height - draw_h > 12) ? draw_h + 12 : height;   // safety net only:
        else if (draw_h > height) draw_h = (draw_h - height > 12) ? draw_h - 12 : height;   // the arcs themselves are smooth
        uint8_t py = BASE_Y - draw_h;
        if (hurt_cool) { hurt_cool--; if (hurt_cool & 4) py = 200; }    // grace period after an OUCH: you blink (and can't be bitten)
        for (i = 0; i < 4; i++) set_sprite_tile(SPR_PLAYER + i, pb + i);
        move_sprite(SPR_PLAYER + 0, px + 8,  py + 16); move_sprite(SPR_PLAYER + 1, px + 16, py + 16);
        move_sprite(SPR_PLAYER + 2, px + 8,  py + 24); move_sprite(SPR_PLAYER + 3, px + 16, py + 24);
    }
}
