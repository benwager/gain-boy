// Battery save API (src/save.c, ROM bank 0)
#ifndef GAINBOY_SAVE_H
#define GAINBOY_SAVE_H

#include <stdint.h>

extern uint8_t best_rank, saved_songs, vstars[6];
extern uint16_t tours;

void save_load(void);
void save_write(void);

#endif /* GAINBOY_SAVE_H */
