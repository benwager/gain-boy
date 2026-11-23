// ============================================================
//  GAIN BOY - shared constants, types, and cross-file declarations
// ============================================================
#ifndef GAINBOY_GAME_H
#define GAINBOY_GAME_H

#include <gb/gb.h>
#include <gb/hardware.h>
#include <rand.h>
#include <stdint.h>
#include "assets.h"

#define BASE_Y     104
#define H_LEDGE    8
#define H_PAD      48
#define DJ_APEX    56
#define DJ_T       20
#define DJ_MIN_T   14
#define DJ_WALL    66
#define KEY_ROW    15
#define PAD_ROW    9
#define LEDGE_ROW  14
#define CROWD_ROW  17
#define NKEYS      8
#define N_SONGS    6
#define KICK_X     1800
#define NECHO      6
#define NSPARK     4
#define VERSION_STR "V1.0"

#define FPS  ((uint8_t)(v->fps + tempo_mod))
#define ROOT ((uint8_t)(songs[song].root + kshift))

#define SPR_PLAYER 0
#define SPR_SPARK  4
#define SPR_GHOST  8
#define SPR_BAD    12
#define SPR_TW     13
#define SPR_TITLE  15
#define SPR_PRESS  24
#define SPR_HINT   29
#define SPR_PICK   30
#define SPR_STARS  32
#define SPR_BIRD   35
#define SPR_MYS    36
#define SPR_DISCO  37
#define SPR_BELL   38
#define BELL_X     12
#define BELL_Y     30

#define A_VERSE  1
#define A_CHORUS 2
#define A_BREAK  3
#define A_FINALE 4
#define A_BIG (arr == A_CHORUS || arr == A_FINALE)

#define start_sfx(k) do { sfx_kind = (k); sfx_i = 0; } while (0)

enum { SURF_FLOOR, SURF_LEDGE, SURF_PAD };
enum { M_BOUNCE, M_STAND, M_SIT, M_WAIT, M_IDLE };

/* One song. Defined in src/songs.c — see the grids there for drum patterns. */
typedef struct {
    uint8_t fps;              /* frames per 16th (~60/fps ≈ BPM/4) */
    uint8_t swing;            /* even 16ths shortened by this many frames */
    uint8_t root;             /* MIDI root of the scale */
    uint8_t minor;            /* 0 = major pent, 1 = minor pent */
    int8_t  prog[4];          /* default 4-chord loop, semitones from root */
    uint8_t pmask;            /* bit n set → prog[n] is a minor triad */
    uint16_t kick, snare, hat, ohat;  /* 16-step masks, bit n = 16th n */
    uint8_t clap;             /* 1 = snare also queues a clap flam */
    uint8_t lead_duty;        /* CH2 duty 0–3 */
    uint8_t lead_env;         /* CH2 envelope byte */
    uint8_t vib;              /* vibrato depth (0 = off) */
    uint8_t echo_t;           /* echo delay in 16ths */
    uint8_t echo1, echo2;     /* envelopes for 1st / 2nd echo tap */
    uint8_t bass_wave;        /* wavetables[] index for bass */
    uint8_t loop_wave;        /* wavetables[] index for melody loops */
    uint8_t kick_click;       /* 1 = noise click layered on kick */
} Song;

/* songs.c */
extern const Song songs[6];
extern const char * const song_names[6];
extern const int8_t pad_semi_maj[4];
extern const int8_t pad_semi_min[4];
extern const uint8_t pad_min_maj;
extern const uint8_t pad_min_min;
extern const uint8_t pad_x[4];
extern const uint8_t pent_maj[5];
extern const uint8_t pent_min[5];
extern const uint8_t mod3[32];
extern const uint8_t key_off_maj[10];
extern const uint8_t key_off_min[10];

/* main.c state */
extern uint8_t para64[65];
extern uint16_t sq64[65];
extern int8_t tempo_mod;
extern int8_t kshift;
extern uint8_t song;
extern uint8_t step, bar, tick, frame_in_step, step_len;
extern uint8_t keys, prev_keys, pressed;
extern uint16_t idle;
extern uint8_t attract;
extern uint8_t px, pmode, height;
extern uint8_t b_from, b_H, b_T, b_t, b_left, b_len, big_queued;
extern uint8_t is_big, dbl;
extern uint16_t loops;
extern uint8_t want_glow, want_crowd, want_lights, want_speak;
extern uint8_t sfx_frames, sfx_kind, sfx_i, q_sfx, cheered, strum_dir, roll_steps;
extern uint8_t lit_key, lit_timer, lit_pad, lit_pad_timer;
extern uint8_t override_bars, pad_chord;
extern uint8_t hist[32], loop[32], loop_on, landed_now, banner, banner_vibe;
extern uint8_t g_x, g_fx, g_tx, g_fh, g_th, g_t, g_T, g_arc, g_y;
extern uint8_t e_on[NECHO], e_due[NECHO], e_note[NECHO], e_env[NECHO];
extern uint8_t stab_frames;
extern uint16_t stab_x[3];
extern uint8_t clap_frames;
extern uint8_t live_x_valid;
extern uint16_t live_x;
extern uint8_t live_frames;
extern uint8_t energy, crowd_frame;
extern uint8_t bad_on, bad_x, bad_dir, bad_squash, bad_wait, bad_t;
extern uint8_t tw_x[2], tw_life[2];
extern uint8_t spk_x[NSPARK], spk_y[NSPARK], spk_life[NSPARK];
extern uint8_t ai_dir, ai_t, attract_shown;
extern const char *ban_text;
extern uint8_t ban_t, ban_x, ban_dirty;
extern uint8_t req_state, req_len, req[6], req_idx, req_show, req_cool, req_dirty;
extern uint16_t req_timer;
extern uint8_t req_done, req_stall;
extern uint8_t pk_type, pk_side, pk_wait, pk_life;
extern uint8_t power_steps, pad_seq, drop_through, down_lock;
extern uint8_t bounce_off, ghost_vis, dbl_q;
extern uint8_t land_kind, down_q, down_cnt, hold_k, sustain;
extern uint8_t stut, stut_s0, stut_prev, sw_cool;
extern uint8_t tgt_s, vis_h, draw_h;
extern uint8_t venue, venue_bgp, thump, shake, fireworks;
extern uint8_t last_k, prev_k, up_run, dn_run, same_run, trill_run, tricks;
extern uint8_t clap_bars, fast_bars;
extern uint8_t sing_n, sing_i, sing[6];
extern uint8_t bell_cool, from_pad;
extern uint8_t bird_on, bird_y, bird_flee, bird_wait, bird_t;
extern int16_t bird_x;
extern uint8_t mys_key, mys_bars, mys_wait, mys_last;
extern const char * const venue_names[3];
extern uint8_t glow_mask, glow_shown, sweet_bar, last_sweet_k;
extern uint8_t bar_notes[16], prev_notes[16];
extern uint8_t riff_n, loop_cool, paused, last_trick;
extern uint8_t cur_wave;

/* tables / sound / player / seq / helpers */
void init_tables(void);
uint8_t phase64(uint8_t t, uint8_t T);
uint16_t nfreq(int16_t m);
void ch1_raw(uint16_t x, uint8_t sweep, uint8_t duty, uint8_t env);
void noise_hit(uint8_t len, uint8_t env, uint8_t poly);
void kick(void);
void snare(void);
void hat(void);
void ohat(void);
void crash(void);
void click(void);
void pan2(uint8_t where);
void lead(uint8_t m, uint8_t env, uint8_t where);
void wave_note(uint8_t m, uint8_t len, uint8_t vol);
void load_wave(uint8_t w);
void use_wave(uint8_t w);
int8_t chord_semi(void);
uint8_t chord_is_minor(void);
uint8_t key_note(uint8_t k);
void calc_glow(void);
void draw_glow(uint8_t mask);
void strum(uint8_t pad, uint8_t env);
void echo_push(uint8_t m);
void draw_key(uint8_t k, uint8_t state);
void draw_pad(uint8_t p, uint8_t lit);
void show_banner(const char *t, uint8_t x, uint8_t steps);
void draw_stage(void);
void draw_speakers(uint8_t t);
void draw_crowd(uint8_t f);
uint8_t letter_tile(char c);
void show_text(uint8_t spr, const char *s, uint8_t x, uint8_t y, uint8_t slots);
void hide_sprites(uint8_t from, uint8_t n);
void spawn_tile(uint8_t x, uint8_t y, uint8_t t);
void spawn_spark(uint8_t x, uint8_t y);
void spawn_star(uint8_t x, uint8_t y);
uint8_t cx(void);
int8_t pad_under(uint8_t x);
uint8_t surface(uint8_t x, uint8_t peak, uint8_t *kind);
uint8_t frames_of(uint8_t from_step, uint8_t n);
uint8_t arc_for(uint8_t T);
void start_bounce(uint8_t len);
uint8_t f2b(void);
void extend_bounce(uint8_t extra_steps, uint8_t rise);
void dbl_jump(void);
void award_star(uint8_t pts);
void do_trick(const char *name, uint8_t x, uint8_t id);
void play_q(uint8_t id);
void sfx_step(uint8_t busy);
void pre_bar(void);
void land(uint8_t pos);
uint8_t ev_x(uint8_t e);
uint8_t ev_h(uint8_t e);
void ghost_land(uint8_t pos);
void start_request(void);
void capture(void);
void loop_toggle(void);
void do_step(uint8_t s);
void reset_play(void);
void park_player(uint8_t hop);
void enter_attract(void);
void draw_version(void);

#endif /* GAINBOY_GAME_H */
