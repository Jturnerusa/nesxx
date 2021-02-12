#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>
#include "config.hxx"

//Forward declaration
namespace rom {class Rom;}
namespace ppu {class Ppu;}

namespace bus {

const int RAMSIZE  = 2048;
const int VRAMSIZE = 2048;

const int RAM_ADDRESS_START       = 0;
const int RAM_ADDRESS_END         = 0x1fff;
const int RAM_ADDRESS_MAX_BITS    = 0x07ff;
const int PPU_ADDRESS_START       = 0x2000;
const int PPU_ADDRESS_END         = 0x3fff;
const int PPU_ADDRESS_MAX_BITS    = 0x2007;
const int IO_ADDRESS_START        = 0x4000;
const int IO_ADDRESS_END          = 0x4017;
const int IO_DISABLED_START       = 0x4018;
const int IO_DISABED_END          = 0x401f;
const int ROM_START               = 0x8000;
const int ROM_END                 = 0xffff;

const int PATTERN_TABLE_START  = 0x0000;
const int PATTERN_TABLE_END    = 0x1fff;
const int NAMETABLE_START      = 0x2000;
const int NAMETABLE_END        = 0x3eff;
const int NAMETABLE_MAX_BITS   = 0x2fff;
const int PALETTE_RAM_START    = 0x3f00;
const int PALETTE_RAM_END      = 0x3fff;
const int PALETTE_RAM_MAX_BITS = 0x3f1f;

const int PATTERN_TABLE_SIZE = 0x1000;
const int NAMETABLE_SIZE     = 0x0400;

class Bus {
private:
    uint16_t truncate_ram_address(uint16_t);
    uint16_t truncate_vram_address(uint16_t);
    uint16_t nametable_mirroring_calculator(uint16_t);
    void process_oam_dma(uint8_t);
    std::array<uint8_t, RAMSIZE>  ram;
    std::array<uint8_t, VRAMSIZE> vram;
public:
    Bus();
    void connect_rom(rom::Rom *);
    void connect_ppu(ppu::Ppu *);
    uint8_t read_ram(uint16_t);
    uint16_t read_ram_16(uint16_t);
    void write_ram(uint16_t, uint8_t);
    void write_ram_16(uint16_t, uint16_t);
    uint8_t read_vram(uint16_t);
    void write_vram(uint16_t, uint8_t);
    void vram_debug_view(int, int);
    rom::Rom *rom;
    ppu::Ppu *ppu;
};

#ifdef UNITTEST

    const int DUMMY_RAMSIZE = 0xffff;
    const int DUMMY_VRAM_SIZE = 0x3fff;

    class DummyBus : Bus {
    public:
        uint8_t read_ram(uint16_t);
        uint16_t read_ram_16(uint16_t);
        void write_ram(uint16_t, uint8_t);
        void write_ram_16(uint16_t, uint16_t);
        uint8_t read_vram(uint16_t);
        void write_vram(uint16_t, uint8_t);
        std::array<uint8_t, DUMMY_RAMSIZE> ram;
        std::array<uint8_t, DUMMY_VRAM_SIZE> vram;
    };

    void run_bus_tests();

#endif

#endif

}