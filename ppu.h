#ifndef PPU_H
#define PPU_H
#define PPU_RAMSIZE 0x3fff
#define OAM_RAMSIZE 256
#include <stdint.h>

struct PPU {
    uint8_t controller;
    uint8_t mask;
    uint8_t status;
    uint8_t oam_address;
    uint8_t oam_data;
    uint8_t scroll;
    uint16_t address;
    int address_write_in_progess;
    uint8_t data;
    uint8_t data_buffer;
    uint8_t oam_dma;
    uint8_t oam_ram[OAM_RAMSIZE]
    uint8_t ram[PPU_RAMSIZE];
    int nmi_interrupt;
    int vblank;
    uint64_t cycles;
    int scanline;
    uint16_t 16shift1;
    uint16_t 16shift2;
    uint8_t 8shift1;
    uint8_t 8shift2;
};
#endif
