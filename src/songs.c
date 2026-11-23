// ============================================================
//  GAIN BOY - songs (ROM bank 1)
//
//  Edit this file to change how a song feels.
//  Each Song entry: tempo, key, chords, drums, lead, echo.
//
//  Drum fields are 16-step bitmasks (one bar of 16ths):
//      step:  0 1 2 3  4 5 6 7  8 9 a b  c d e f
//      beat:  1 e & a  2 e & a  3 e & a  4 e & a
//  A set bit = that drum fires on that 16th (see seq.c / sound.c).
// ============================================================
#pragma bank 1
#include "game.h"

/* Which 16ths of a bar fire (bit 0 = first 16th / downbeat). */
#define S0  0x0001u
#define S1  0x0002u
#define S2  0x0004u
#define S3  0x0008u
#define S4  0x0010u
#define S5  0x0020u
#define S6  0x0040u
#define S7  0x0080u
#define S8  0x0100u
#define S9  0x0200u
#define S10 0x0400u
#define S11 0x0800u
#define S12 0x1000u
#define S13 0x2000u
#define S14 0x4000u
#define S15 0x8000u

const Song songs[6] = {

    /* -------- SUNNY · ~112 BPM · C major pent · pop --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x... .... x.x. ....
     * snare: .... x... .... x...
     * hat:   ..x. ..x. ..x. ..x.
     */
    {
        .fps = 8, .swing = 0,
        .root = 60, .minor = 0,                 /* MIDI C4, major */
        .prog = { 0, 7, 9, 5 }, .pmask = 0x04,  /* I–V–vi–IV (vi minor) */
        .kick  = S0 | S8 | S10,
        .snare = S4 | S12,
        .hat   = S2 | S6 | S10 | S14,
        .ohat  = 0,
        .clap = 0,
        .lead_duty = 1, .lead_env = 0xB3, .vib = 0,
        .echo_t = 3, .echo1 = 0x52, .echo2 = 0x31,
        .bass_wave = 3, .loop_wave = 4,
        .kick_click = 1,
    },

    /* -------- NIGHT · ~90 BPM · A minor pent · swung boom-bap --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x... ...x .x.. ....
     * snare: .... x... .... x...
     * hat:   x.x. x.x. x.x. x.x.     (+ swing via .swing)
     */
    {
        .fps = 10, .swing = 2,
        .root = 57, .minor = 1,                 /* MIDI A3, minor */
        .prog = { 0, 8, 3, 10 }, .pmask = 0x01, /* i–VI–III–VII */
        .kick  = S0 | S7 | S9,
        .snare = S4 | S12,
        .hat   = S0 | S2 | S4 | S6 | S8 | S10 | S12 | S14,
        .ohat  = 0,
        .clap = 1,
        .lead_duty = 2, .lead_env = 0x95, .vib = 2,
        .echo_t = 4, .echo1 = 0x62, .echo2 = 0x41,
        .bass_wave = 2, .loop_wave = 3,
        .kick_click = 1,
    },

    /* -------- RAVE · ~128 BPM · A minor pent · four-on-the-floor --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x... x... x... x...
     * snare: .... x... .... x...
     * ohat:  ..x. ..x. ..x. ..x.
     */
    {
        .fps = 7, .swing = 0,
        .root = 57, .minor = 1,
        .prog = { 0, 0, 8, 10 }, .pmask = 0x03, /* i–i–VI–VII */
        .kick  = S0 | S4 | S8 | S12,
        .snare = S4 | S12,
        .hat   = 0,
        .ohat  = S2 | S6 | S10 | S14,
        .clap = 1,
        .lead_duty = 0, .lead_env = 0xC2, .vib = 0,
        .echo_t = 3, .echo1 = 0x62, .echo2 = 0x41,
        .bass_wave = 6, .loop_wave = 5,
        .kick_click = 0,
    },

    /* -------- DREAM · ~100 BPM · F major pent · soft / floaty --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x... .... x... ....
     * hat:   ..x. ..x. ..x. ..x.
     */
    {
        .fps = 9, .swing = 1,
        .root = 65, .minor = 0,                 /* MIDI F4 */
        .prog = { 0, 5, 9, 7 }, .pmask = 0x04,  /* I–IV–vi–V */
        .kick  = S0 | S8,
        .snare = 0,
        .hat   = S2 | S6 | S10 | S14,
        .ohat  = 0,
        .clap = 0,
        .lead_duty = 2, .lead_env = 0x86, .vib = 3,
        .echo_t = 6, .echo1 = 0x62, .echo2 = 0x41,
        .bass_wave = 1, .loop_wave = 2,
        .kick_click = 0,
    },

    /* -------- FUNK · ~112 BPM · G minor pent · syncopated --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x..x ...x ..x. ....
     * snare: .... x... .... x...
     * hat:   x.x. x.x. x.x. x.x.
     * ohat:  .... .... .... ..x.     (last 16th / "a" of 4)
     */
    {
        .fps = 8, .swing = 1,
        .root = 55, .minor = 1,                 /* MIDI G3 */
        .prog = { 0, 5, 0, 10 }, .pmask = 0x07, /* i–iv–i–VII */
        .kick  = S0 | S3 | S7 | S10,
        .snare = S4 | S12,
        .hat   = S0 | S2 | S4 | S6 | S8 | S10 | S12 | S14,
        .ohat  = S14,
        .clap = 0,
        .lead_duty = 1, .lead_env = 0xA2, .vib = 0,
        .echo_t = 3, .echo1 = 0x52, .echo2 = 0x31,
        .bass_wave = 5, .loop_wave = 4,
        .kick_click = 1,
    },

    /* -------- SPOOKY · ~100 BPM · E minor pent · sparse / wobbly --------
     *        1e&a 2e&a 3e&a 4e&a
     * kick:  x... .... x... ....
     * snare: .... .... .... x...
     * hat:   ..x. ..x. ..x. ..x.
     */
    {
        .fps = 9, .swing = 1,
        .root = 52, .minor = 1,                 /* MIDI E3 */
        .prog = { 0, 1, 0, 8 }, .pmask = 0x05,  /* i–bII–i–VI */
        .kick  = S0 | S8,
        .snare = S12,
        .hat   = S2 | S6 | S10 | S14,
        .ohat  = 0,
        .clap = 0,
        .lead_duty = 2, .lead_env = 0x86, .vib = 3,
        .echo_t = 6, .echo1 = 0x62, .echo2 = 0x41,
        .bass_wave = 1, .loop_wave = 2,
        .kick_click = 0,
    },
};

const char * const song_names[6] = {
    "SUNNY", "NIGHT", "RAVE", "DREAM", "FUNK", "SPOOKY"
};

/* Chord pads (left→right), semitones from song root.
 * Major: I IV V vi · Minor: i iv VII VI */
const int8_t pad_semi_maj[4] = { 0, 5, 7, 9 };
const uint8_t pad_min_maj = 0x08;       /* bit n set → pad n is minor */
const int8_t pad_semi_min[4] = { 0, 5, 10, 8 };
const uint8_t pad_min_min = 0x03;

const uint8_t pad_x[4] = { 24, 56, 88, 120 };   /* left edge of each 16px pad */

/* Floor key → scale degree (semitones). Keys 0..4, then octave up. */
const uint8_t pent_maj[5] = { 0, 2, 4, 7, 9 };
const uint8_t pent_min[5] = { 0, 3, 5, 7, 10 };
const uint8_t key_off_maj[10] = { 0, 2, 4, 7, 9, 12, 14, 16, 19, 21 };
const uint8_t key_off_min[10] = { 0, 3, 5, 7, 10, 12, 15, 17, 19, 22 };

const uint8_t mod3[32] = { /* n%3 without dividing */
    0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1,2, 0,1
};
