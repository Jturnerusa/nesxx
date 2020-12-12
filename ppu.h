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
    uint8_t address;
    uint8_t data;
    uint8_t oam_dma;
    uint8_t oam_ram[OAM_RAMSIZE]
    uint8_t ram[PPU_RAMSIZE];
    int nmi_interrupt;
};
#endif
