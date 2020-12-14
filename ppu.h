#ifndef PPU_H
#define PPU_H

#include <stdint.h>

#define PPU_RAMSIZE 0x3fff
#define OAM_RAMSIZE 256
#define SCREEN_WIDTH
#define SCREEN_HEIGHT
#define PATTERN_TABLE_0 0x0000
#define PATTERN_TABLE_1 0x1000
#define NAMETABLE_0 0x2000
#define NAMETABLE_1 0x2400
#define NAMETABLE_2 0x2800
#define NAMETABLE_3 0x2c00


struct PPU {
    uint8_t controller;
    uint8_t mask;
    uint8_t status;
    uint8_t oam_address;
    uint8_t oam_data;
    uint8_t scroll;
    int scroll_io_in_progress;
    uint16_t address;
    int address_write_in_progess;
    uint8_t data;
    uint8_t data_buffer;
    uint8_t ram[PPU_RAMSIZE];
    uint8_t oam_ram[OAM_RAMSIZE]
    int nmi_interrupt;
    int vblank;
    uint64_t cycles;
    int scanline;
    int frame_data[SCREEN_WIDTH][SCREEN_HEIGHT];
};
#endif
