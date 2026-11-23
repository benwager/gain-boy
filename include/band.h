// Power-ups, drum kit, loops, progressions (src/band.c, ROM bank 0)
#ifndef GAINBOY_BAND_H
#define GAINBOY_BAND_H

#include <stdint.h>

enum { I_MELODY, I_DRUMS, I_BASS };

extern uint8_t inst, dloop_on, bloop_on, solo_used, hurt_cool;
extern uint8_t inst_bars, fx_id, fx_bars, loop_bars, key_up;
extern int8_t cur_prog[4];
extern uint8_t cur_pmask;

void drum_hit(uint8_t k);
void solo_hit(void);
void fuzz_ouch(void);
void fuzz_touch(void);
void bass_hit(uint8_t k, uint8_t fps);
uint8_t inst_mask(uint8_t t, uint8_t melody_mask);
void fx_power(void);
void fx_pad(uint8_t p);
void inst_power(uint8_t i);
void band_bar(void);
void inst_set(uint8_t i);
char inst_letter(void);
uint8_t band_capture(const uint8_t *hist);
void band_clear(void);
uint8_t band_step(uint8_t pos, uint8_t muted, uint8_t fps);
void pick_prog(void);

#endif /* GAINBOY_BAND_H */
