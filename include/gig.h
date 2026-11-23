// Tour / scoring / pause API (src/gig.c, ROM bank 0)
#ifndef GAINBOY_GIG_H
#define GAINBOY_GIG_H

#include <stdint.h>

extern uint16_t score, best_score;
extern uint8_t mult, hud_dirty, gstate, gig_last, arr, countin, sweet_named, gig_bar;
extern uint8_t jam, level, mood;

void banner_bg(const char *t);
void show_tip(const char *l1, const char *l2);
void mood_add(int8_t n);
void mood_note(uint8_t k);
void add_score(uint8_t base);
void sub_score(uint8_t n);
void hype_tick(void);
void hud_draw(void);
void gig_start(uint8_t l);
void gig_new_run(void);
void gig_jam(void);
void gig_end(void);
void gig_bar_tick(void);
uint8_t gig_frame(uint8_t pressed);
void best_text(char *b);
uint8_t songs_unlocked(void);
void pause_open(void);
uint8_t pause_frame(uint8_t pressed);

#endif /* GAINBOY_GIG_H */
