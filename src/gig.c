// ============================================================
//  GAIN BOY - the tour (ROM bank 0)
//  Stages, gig clock and arrangement, scoring and multiplier,
//  crowd mood, HUD, banners and tips, results and final cards,
//  retries, Jam mode, pause menu. Instrument lives in main.c (bank 1).
// ============================================================
#include "game.h"
#include "gig.h"
#include "band.h"
#include "save.h"

uint8_t tip2_on = 0;
// a two-line tip: both lines appear together, so half a tip can never show on its own
// banners and tips are drawn into the background (row 4, and row 6 for a tip's 2nd line): no sprite cut-offs
void bg_line(uint8_t row, const char *t) {
    uint8_t buf[18], n = 0, i;
    for (i = 0; i < 18; i++) buf[i] = T_BLANK;
    if (t) { while (t[n] && n < 18) n++; uint8_t x = (18 - n) >> 1; for (i = 0; i < n; i++) buf[x + i] = letter_tile(t[i]); }
    set_bkg_tiles(1, row, 18, 1, buf);
}
void banner_bg(const char *t) { bg_line(4, t); }
void show_tip(const char *l1, const char *l2) {   // two lines that always appear together
    show_banner(l1, 0, 14);
    if (l2) { bg_line(6, l2); tip2_on = 1; }
}
uint8_t jam = 0;                 // JAM mode: endless free play, no timer, no results
uint8_t run_key = 0;             // each tour starts in a random key
uint8_t songs_unlocked(void) {   // SELECT cycles through these: bigger gigs (now or ever) unlock more
    uint8_t now = level + 2 > 6 ? 6 : level + 2;            // each level opens one more song
    return jam ? saved_songs : (now > saved_songs ? now : saved_songs);
}

// ================= TUNING: gig lengths (bars) and star thresholds per venue =================
// ================= THE TOUR: six levels =================
const char * const level_names[6] = { "GARAGE", "CLUB", "BIG STAGE", "ARENA", "STADIUM", "THE MOON" };
const uint8_t level_venue[6] = { 0, 1, 2, 1, 2, 2 };     // which stage it looks like (the moon gets its own sky)
const uint8_t level_song[6]  = { 0, 1, 2, 4, 5, 3 };     // SUNNY NIGHT RAVE FUNK SPOOKY DREAM
const uint8_t level_bars[6]  = { 24, 24, 32, 32, 32, 32 };
const uint16_t star_lv[6][3] = { {80, 220, 350}, {90, 200, 320}, {100, 220, 340},
                                 {150, 260, 380}, {180, 290, 420}, {220, 330, 460} };   // later stages need real playing, not mashing
uint8_t level = 0, cleared = 0, booed = 0, stars_now = 0, continues = 2;
void blip(uint8_t midi_hi, uint8_t env);
// ================= THE CROWD'S MOOD: keep them with you, or get booed off =================
uint8_t mood = 70, same_k = 255, same_ct = 0, boo_warned = 0, silent_bars = 0;
void mood_add(int8_t n) {
    if (attract || jam || gstate) return;
    int16_t m = (int16_t)mood + n;
    mood = m < 0 ? 0 : m > 100 ? 100 : (uint8_t)m;
}
void mood_note(uint8_t k) {           // every note you land: variety pleases, the same note over and over bores
    if (k == same_k) {
        if (++same_ct >= 10) {                                // repeats are fine; only a LOT of the same note bores them
            if (same_ct & 1) mood_add(-1);
            if (same_ct == 10 && !ban_t) show_banner("BORING!", 0, 10);
        }
    } else { same_k = k; same_ct = 0; mood_add(1); }
}
const uint8_t  gig_bars[3]  = { 24, 24, 32 };
// measured with bots: a masher scores ~150 a gig, a casual wanderer ~300, a musical player (requests,
// chord tones on the beat, riffs) ~450-750. Holding B and walking scores almost nothing.
const uint16_t star_pts[3][3] = { {80, 220, 350}, {90, 200, 320}, {100, 220, 340} };   // re-measured without requests (v1.9)
const uint16_t rank_pts[3] = { 300, 600, 900 };     // OPENER, HEADLINER, LEGEND
const char * const rank_names[4] = { "ROADIE", "OPENER", "HEADLINER", "LEGEND!" };

uint16_t score = 0, gig_base = 0, best_score = 0;
uint8_t sdig[5];                 // score as decimal digits (cheap to draw, no division per frame)
uint8_t mult = 1, hud_dirty = 31; // bit0 score, bit1 multiplier, bit2 time, bit3 instrument
uint8_t sweet_named = 0;         // the first few sweet notes are named on screen
uint8_t gig_bar = 0, gig_len = 24, gig_last = 0;
uint8_t gstate = 0;              // 0 playing, 1 gig results, 2 final card
// ---- song arrangement: every gig is a song ----
enum { ARR_INTRO, ARR_VERSE, ARR_CHORUS, ARR_BREAK, ARR_FINALE };
uint8_t arr = ARR_VERSE, countin = 0;
uint8_t section_of(uint8_t gb, uint8_t L) {      // gb counts bars from 1
    if (gb <= 2) return ARR_INTRO;
    if (gb > L - 8) return ARR_FINALE;
    if (gb > L - 10) return ARR_BREAK;
    if (gb > L - 14) return ARR_CHORUS;
    return ARR_VERSE;
}
uint8_t rating = 0, new_best = 0;
uint16_t res_t = 0, res_pts = 0, res_shown = 0;

// ---------------- score ----------------
void digits_of(uint16_t v, uint8_t *d) {
    for (int8_t i = 4; i >= 0; i--) { d[i] = v % 10; v /= 10; }
}
uint8_t hype = 0;                // feeds the multiplier: only real moments raise it, it drains on its own
void set_mult(void) {
    uint8_t nm = 1 + (hype >= 20) + (hype >= 45) + (hype >= 75);
    if (nm != mult) { mult = nm; hud_dirty |= 2; }
}
void hype_tick(void) { if (hype) { hype--; set_mult(); } }      // called every other step
void sub_score(uint8_t n) {             // a penalty: points off, and the multiplier resets
    if (attract || gstate) return;
    score = (score > n) ? score - n : 0;
    digits_of(score, sdig); hype = 0; set_mult(); hud_dirty |= 1;
    mood_add(-15);                                        // getting bitten costs you some of the crowd
}
void add_score(uint8_t base) {
    if (attract || gstate) return;
    if (base >= 2) mood_add((int8_t)(base / 3 + 1));     // good moments win the crowd over
    if (base >= 2) { uint8_t h = base > 20 ? 20 : base; hype = (hype + h > 100) ? 100 : hype + h; set_mult(); }
    uint16_t p = (uint16_t)base * mult;
    if (score > 60000) return;
    score += p;
    // add to the digit array with carries (no division)
    uint8_t h = 0; while (p >= 100) { p -= 100; h++; }
    uint8_t t = 0; while (p >= 10) { p -= 10; t++; }
    uint8_t add[3]; add[0] = (uint8_t)p; add[1] = t; add[2] = h;
    uint8_t carry = 0;
    for (uint8_t i = 0; i < 5; i++) {
        uint8_t v = sdig[4 - i] + carry + (i < 3 ? add[i] : 0);
        carry = 0; while (v >= 10) { v -= 10; carry++; }
        sdig[4 - i] = v;
    }
    hud_dirty |= 1;
}

// ---------------- HUD: top row = score, multiplier, time left ----------------
void hud_draw(void) {
    uint8_t t[11], i;
    if (attract) { hud_dirty = 0; return; }
    if (hud_dirty & 1) { for (i = 0; i < 5; i++) t[i] = letter_tile('0' + sdig[i]); set_bkg_tiles(0, 0, 5, 1, t); }
    if (hud_dirty & 2) { t[0] = letter_tile('X'); t[1] = letter_tile('0' + mult); set_bkg_tiles(6, 0, 2, 1, t); }
    if (hud_dirty & 16) {                                   // L = a loop is playing (blinks in its last bar)
        extern uint8_t loop_on, dloop_on, bloop_on, loop_bars;
        uint8_t on = loop_on || dloop_on || bloop_on;
        t[0] = (on && !(loop_bars == 1 && (step & 4))) ? letter_tile('L') : T_BLANK; set_bkg_tiles(10, 0, 1, 1, t);
    }
    if (hud_dirty & 8) { t[0] = (inst_bars == 1 && (step & 4)) ? T_BLANK : letter_tile(inst_letter()); set_bkg_tiles(9, 0, 1, 1, t); }   // M / D / B
    if ((hud_dirty & 4) && jam) {
        for (i = 0; i < 9; i++) t[i] = T_BLANK;
        t[3] = letter_tile('J'); t[4] = letter_tile('A'); t[5] = letter_tile('M');
        set_bkg_tiles(11, 0, 9, 1, t);
    } else if (hud_dirty & 4) {
        uint8_t left = gig_len - gig_bar;
        uint8_t cells = (uint8_t)(((uint16_t)left * 9 + gig_len - 1) / gig_len);
        uint8_t blink = (left <= 2) && (step & 4);          // nearly over: the bar flashes
        for (i = 0; i < 9; i++) t[i] = (i < cells && !blink) ? T_MFULL : T_MEMPTY;
        set_bkg_tiles(11, 0, 9, 1, t);
    }
    hud_dirty = 0;
}

// ---------------- text on the background ----------------
void bg_text(uint8_t y, const char *s) {                    // centred
    uint8_t n = 0; while (s[n]) n++;
    uint8_t x = (20 - n) >> 1, t;
    for (uint8_t i = 0; i < n; i++) { t = letter_tile(s[i]); set_bkg_tiles(x + i, y, 1, 1, &t); }
}
void bg_number(uint8_t y, const char *label, const uint8_t *d) {
    char b[14]; uint8_t n = 0;
    while (label[n]) { b[n] = label[n]; n++; }
    b[n++] = ' ';
    for (uint8_t i = 0; i < 5; i++) b[n++] = '0' + d[i];
    b[n] = 0; bg_text(y, b);
}
void clear_rows(uint8_t from, uint8_t to) {
    uint8_t row[20];
    for (uint8_t c = 0; c < 20; c++) row[c] = T_BLANK;
    for (uint8_t r = from; r <= to; r++) set_bkg_tiles(0, r, 20, 1, row);
}
void blip(uint8_t midi_hi, uint8_t env) {                   // little pulse-2 note for the result jingles
    static const uint16_t f[6] = {1798, 1849, 1899, 1923, 1954, 1986};   // C6 E6 G6 A6 C7 E7-ish
    NR51_REG |= 0x22; NR21_REG = 0x80; NR22_REG = env;
    NR23_REG = f[midi_hi] & 0xFF; NR24_REG = 0x80 | (f[midi_hi] >> 8);
}

// ---------------- gig flow ----------------
void gig_start(uint8_t l) {
    level = l; venue = level_venue[l];
    draw_stage();
    gig_bar = 0; gig_len = level_bars[l]; gig_base = score; gig_last = 0;
    reset_play();
    kshift = (int8_t)(run_key + (l & 3) * 2);               // a random key per tour, lifted as you go
    song = level_song[l]; pick_prog();                      // each level's song, with a fresh chord progression
    mood = 70; same_k = 255; same_ct = 0; boo_warned = 0; booed = 0; silent_bars = 0; stars_now = 0;
    bar = 255; step = 15; frame_in_step = step_len - 1;     // start cleanly on a bar line
    gstate = 0; hud_dirty = 63; hype = 0; mult = 1;
    arr = ARR_INTRO; countin = 0;
}
void gig_new_run(void) {
    jam = 0; cleared = 0; continues = 2; run_key = rand() % 3; tours++;
    score = 0; for (uint8_t i = 0; i < 5; i++) sdig[i] = 0;
    mult = 1; hype = 0; new_best = 0; sweet_named = 0;
    gig_start(0);
}
void gig_jam(void) {             // free play on the biggest stage you've reached, every song you've unlocked
    score = 0; for (uint8_t i = 0; i < 5; i++) sdig[i] = 0;
    run_key = rand() % 3;
    uint8_t lv = 0; for (uint8_t i = 0; i < 6; i++) if (vstars[i]) lv = i + 1;
    gig_start(lv > 5 ? 5 : lv);                             // the furthest stage you've reached
    jam = 1; mood = 100;
    show_banner("JAM!", 0, 16);
}
void gig_end(void) {
    uint16_t pts = score - gig_base;
    rating = pts >= star_lv[level][2] ? 3 : pts >= star_lv[level][1] ? 2 : pts >= star_lv[level][0] ? 1 : 0;
    if (booed) rating = 0;                                  // booed off: no stars, the tour is over
    gstate = 1; res_t = 0;
    if (rating > vstars[level]) vstars[level] = rating;
    if (rating >= 1) { cleared++; uint8_t ns = level + 3; if (ns > 6) ns = 6; if (ns > saved_songs) saved_songs = ns; }   // each level you pass unlocks a song for good
    save_write();
    // the last hit: kick + crash + a big ringing note, then quiet
    NR32_REG = 0x00;
    NR10_REG = 0x19; NR11_REG = 0x80; NR12_REG = 0xF2; NR13_REG = 0x08; NR14_REG = 0x87;
    NR41_REG = 0; NR42_REG = 0xD5; NR43_REG = 0x01; NR44_REG = 0x80;
    blip(0, 0xF6);
    loop_on = 0; req_state = 0; req_dirty = 1; pk_type = 0; fireworks = 0; power_steps = 0;
    OBP0_REG = (level == 5) ? 0x1B : 0xE4; SCX_REG = 0; SCY_REG = 0; shake = 0;
    hide_sprites(4, 36);                                    // everything but the boombox
    park_player(0);
    clear_rows(1, 9);                                       // pads hidden while the card is up
    bg_text(2, booed ? "BOOED OFF!" : "GIG OVER");
    res_pts = pts; res_shown = 0;
    uint8_t d[5]; digits_of(0, d);
    bg_number(4, "SCORE", d);
    hud_dirty = 7;
}
void final_card(void) {
    gstate = 2; res_t = 0;
    if (score > best_score) { best_score = score; new_best = 1; }
    uint8_t r = cleared >= 6 ? 3 : cleared >= 4 ? 2 : cleared >= 2 ? 1 : 0;   // your rank: how far the tour went
    if (r > best_rank) best_rank = r;
    save_write();
    clear_rows(1, 9);
    bg_text(2, rank_names[r]);
    bg_number(4, "TOTAL", sdig);
    uint8_t d[5]; digits_of(best_score, d);
    bg_number(6, "BEST", d);
    if (new_best) bg_text(8, "NEW BEST!");
}
// called once per bar while playing
void gig_bar_tick(void) {
    gig_bar++; hud_dirty |= 4;
    band_bar();
    if (!jam && !attract && gig_bar > 3) {                  // the crowd's patience (not during the count-in and intro)
        uint8_t n = 0; for (uint8_t i = 0; i < 16; i++) if (prev_notes[i] != 255) n++;
        mood_add(-(int8_t)(2 + (level >> 1)));              // they slowly want more (a little more at bigger gigs)
        if (n == 0) { if (++silent_bars >= 2) mood_add(-3); } else silent_bars = 0;   // one quiet bar is a gap; two is a lull
        if (mood < 20 && !boo_warned) { boo_warned = 1; show_banner("BOO!", 0, 12); NR41_REG = 0; NR42_REG = 0x93; NR43_REG = 0x77; NR44_REG = 0xC0; }
        if (mood >= 35) boo_warned = 0;
        if (mood == 0) { booed = 1; gig_end(); return; }    // everyone's gone: booed off
    }
    if (tip2_on) { tip2_on = 0; bg_line(6, 0); }                // the tip's second line goes with it
    if (jam && gig_bar > gig_len) gig_bar = 3;           // JAM: verse, chorus, break, finale... round again, forever
    if (gig_bar > gig_len) { gig_end(); return; }
    gig_last = (gig_bar == gig_len);
    countin = (gig_bar == 1);                           // bar 1: "1 2 3 4" (main.c shows the numbers)
    uint8_t a = section_of(gig_bar, gig_len), was = arr;
    arr = a;
    if (gig_bar == 2) show_banner(level_names[level], 0, 14);
    else if (a != was) {                                // the song moves on: tell the player
        if (a == ARR_VERSE) show_banner("VERSE", 0, 10);
        else if (a == ARR_CHORUS) show_banner("CHORUS!", 0, 12);
        else if (a == ARR_BREAK) { if (level == 0 && !jam && req_state == 0) show_tip("SELECT", "= LOOP"); else show_banner("BREAK", 0, 14); }
        else if (a == ARR_FINALE) show_banner("FINALE!", 0, 12);
    }
    else if (venue == 2 && gig_bar == gig_len - 3) { show_banner("ENCORE!", 0, 24); fireworks = 64; power_steps = 64; }
    else if (gig_last && !ban_t && !jam) show_banner("LAST BAR!", 0, 12);
    else if (level == 1 && !jam && !ban_t && req_state == 0) {   // the club: teach the wall switches
        if (gig_bar == 4) show_tip("AA LEFT", "= STAND");
        else if (gig_bar == 6) show_tip("HOLD DOWN", "BREAKDOWN");
        else if (gig_bar == 8) show_tip("AA RIGHT", "= KEY UP");
    }
    else if (level == 0 && !jam && !ban_t && req_state == 0) {  // first gig: teach the basics, one line per bar
        if (gig_bar == 4) show_tip("A = JUMP", 0);
        else if (gig_bar == 6) show_tip("STAR KEYS", "ON BEAT!");
        else if (gig_bar == 8) show_tip("AA = PADS", 0);
        else if (gig_bar == 10) show_tip("JUMP ON", "FUZZBALLS");
    }
}
// called every frame while a results card is up. returns 2 when the run is over (back to the demo)
uint8_t gig_frame(uint8_t pressed) {
    res_t++;
    if ((res_t & 15) == 0) { crowd_frame = 255; draw_crowd((res_t & 16) ? 0x21 : 0x12); park_player((res_t & 16) ? 6 : 0); }   // the crowd cheers, the boombox bobs
    if (gstate == 1) {
        // the score counts up, ticking
        if (res_shown < res_pts && (res_t & 1)) {
            uint16_t gap = res_pts - res_shown;
            res_shown += (gap >> 3) + 1;
            uint8_t d[5]; digits_of(res_shown, d);
            bg_number(4, "SCORE", d);
            NR41_REG = 62; NR42_REG = 0x51; NR43_REG = 0x00; NR44_REG = 0xC0;
        }
        // stars pop in one at a time
        for (uint8_t i = 0; i < 3; i++) if (res_t == 50 + i * 24) {
            uint8_t t = (i < rating) ? T_STAR : T_SKYDOT;
            set_bkg_tiles(7 + i * 3, 6, 1, 1, &t);
            if (i < rating) blip(i * 2, 0xC3); else { NR41_REG = 58; NR42_REG = 0x50; NR43_REG = 0x00; NR44_REG = 0xC0; }
        }
        if (res_t == 130) {
            if (rating == 0) {
                bg_text(8, booed ? "THE CROWD LEFT" : "NOT THIS TIME");
                if (continues && !jam) bg_text(9, continues == 2 ? "A=RETRY B=END" : "A=LAST TRY B=END");
            }
            else if (level < 5) {
                char b[16]; const char *n = level_names[level + 1]; uint8_t i = 0, j = 0;
                b[i++] = 'N'; b[i++] = 'E'; b[i++] = 'X'; b[i++] = 'T'; b[i++] = ' ';
                while (n[j] && i < 15) b[i++] = n[j++];
                b[i] = 0; bg_text(8, b); blip(4, 0xC4);
            }
            else { bg_text(8, "WHAT A SHOW!"); blip(5, 0xC5); }
        }
        if (rating == 0 && continues && !jam && res_t > 150) {     // a continue: try this stage again
            if (pressed & J_A) {
                continues--; score = gig_base; digits_of(score, sdig);   // the failed attempt's points go
                gig_start(level);
                show_banner(continues ? "RETRY!" : "LAST TRY!", 0, 14);
            } else if ((pressed & (J_B | J_START)) || res_t > 900) final_card();
        }
        else if (res_t > 150 && ((pressed & (J_A | J_START)) || res_t > 520)) {
            if (rating >= 1 && level < 5) gig_start(level + 1);
            else final_card();
        }
    } else {
        if (res_t == 20 || res_t == 28 || res_t == 36 || res_t == 48) blip(res_t == 20 ? 0 : res_t == 28 ? 2 : res_t == 36 ? 4 : 5, 0xC4);
        if (res_t > 120 && ((pressed & (J_A | J_START)) || res_t > 600)) { gstate = 0; return 2; }
    }
    return 1;
}
void best_text(char *b) {            // "HI 01234" for the title screen
    uint8_t d[5]; digits_of(best_score, d);
    b[0] = 'H'; b[1] = 'I'; b[2] = ' ';
    for (uint8_t i = 0; i < 5; i++) b[3 + i] = '0' + d[i];
    b[8] = 0;
}

// ============================================================
//  PAUSE MENU (START): RESUME / SONG (left-right) / BOUNCE ON-OFF / QUIT
// ============================================================
uint8_t pm_sel = 0;
void text_at(uint8_t x, uint8_t y, const char *s, uint8_t width) {   // left-aligned, padded with blanks
    uint8_t t, i = 0;
    for (; s[i] && i < width; i++) { t = letter_tile(s[i]); set_bkg_tiles(x + i, y, 1, 1, &t); }
    for (; i < width; i++) { t = T_BLANK; set_bkg_tiles(x + i, y, 1, 1, &t); }
}
void pause_draw(void) {
    for (uint8_t i = 0; i < 3; i++) { uint8_t t = (i == pm_sel) ? T_STAR : T_BLANK; set_bkg_tiles(3, 4 + i, 1, 1, &t); }
    text_at(5, 4, "RESUME", 6);
    text_at(5, 5, "SONG", 4);  text_at(10, 5, song_names[song], 6);
    text_at(5, 6, "QUIT", 4);
}
void pause_open(void) {
    pm_sel = 0;
    hide_sprites(4, 36);                                    // a clean menu (the game redraws its sprites on resume)
    clear_rows(1, 9);
    bg_text(2, "PAUSED");
    pause_draw();
}
// returns 0 = stay paused, 1 = resume, 2 = quit to the title
uint8_t pause_frame(uint8_t pressed) {
    if (pressed & J_START) return 1;
    if ((pressed & J_UP) && pm_sel) { pm_sel--; pause_draw(); blip(0, 0x41); }
    if ((pressed & J_DOWN) && pm_sel < 2) { pm_sel++; pause_draw(); blip(0, 0x41); }
    uint8_t lr = pressed & (J_LEFT | J_RIGHT);
    if ((pressed & J_A) || lr) {
        if (pm_sel == 0 && (pressed & J_A)) return 1;
        if (pm_sel == 2 && (pressed & J_A)) return 2;
        if (pm_sel == 1) {                                   // change song: every song you've unlocked
            uint8_t n = songs_unlocked();
            song = (pressed & J_LEFT) ? (song ? song - 1 : n - 1) : (song + 1) % n;
            pick_prog(); cur_wave = 255; calc_glow(); glow_shown = 0xFE;
            pause_draw(); blip(2, 0x82);
        }
    }
    return 0;
}
