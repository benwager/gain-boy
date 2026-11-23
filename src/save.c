// ============================================================
//  GAIN BOY - battery save (MBC1 + RAM + BATTERY, 8KB SRAM at 0xA000)
//  high score, best rank, best stars per stage, unlocked songs, tours played.
//  16 bytes + version + checksum; missing/corrupt save starts fresh.
// ============================================================
#include <gb/gb.h>
#include <stdint.h>
#include "save.h"
#include "gig.h"   /* best_score */

uint8_t best_rank = 0, saved_songs = 2, vstars[6] = {0, 0, 0, 0, 0, 0};
uint16_t tours = 0;

#define SRAM ((volatile uint8_t *)0xA000)
#define RAM_ON  (*(volatile uint8_t *)0x0000 = 0x0A)
#define RAM_OFF (*(volatile uint8_t *)0x0000 = 0x00)

static uint8_t sum_of(volatile uint8_t *p) { uint8_t s = 0x5A; for (uint8_t i = 0; i < 15; i++) s += p[i] ^ (i * 7); return s; }

void save_load(void) {
    RAM_ON;
    if (SRAM[0] == 'G' && SRAM[1] == 'B' && SRAM[2] == 2 && SRAM[15] == sum_of(SRAM)) {   // save format v2: six levels
        best_score = SRAM[3] | (SRAM[4] << 8);
        best_rank = SRAM[5]; saved_songs = SRAM[6];
        for (uint8_t i = 0; i < 6; i++) vstars[i] = SRAM[7 + i];
        tours = SRAM[13] | (SRAM[14] << 8);
        if (saved_songs < 2 || saved_songs > 6) saved_songs = 2;
    }
    RAM_OFF;
}
void save_write(void) {
    RAM_ON;
    SRAM[0] = 'G'; SRAM[1] = 'B'; SRAM[2] = 2;
    SRAM[3] = best_score & 0xFF; SRAM[4] = best_score >> 8;
    SRAM[5] = best_rank; SRAM[6] = saved_songs;
    for (uint8_t i = 0; i < 6; i++) SRAM[7 + i] = vstars[i];
    SRAM[13] = tours & 0xFF; SRAM[14] = tours >> 8;
    SRAM[15] = sum_of(SRAM);
    RAM_OFF;
}
