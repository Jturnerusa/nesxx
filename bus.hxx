#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>
#include "config.hxx"

//Forward declaration
class Rom;
class Ppu;
class Controller;

class Bus {
public:
    static const int RAM_ADDRESS_START       = 0;
    static const int RAM_ADDRESS_END         = 0x1fff;
    static const int RAM_ADDRESS_MAX_BITS    = 0x07ff;
    static const int PPU_ADDRESS_START       = 0x2000;
    static const int PPU_ADDRESS_END         = 0x3fff;
    static const int PPU_ADDRESS_MAX_BITS    = 0x2007;
    static const int IO_ADDRESS_START        = 0x4000;
    static const int IO_ADDRESS_END          = 0x4017;
    static const int IO_DISABLED_START       = 0x4018;
    static const int IO_DISABED_END          = 0x401f;
    static const int ROM_START               = 0x8000;
    static const int ROM_END                 = 0xffff;
    static const int PATTERN_TABLE_START  = 0x0000;
    static const int PATTERN_TABLE_END    = 0x1fff;
    static const int NAMETABLE_START      = 0x2000;
    static const int NAMETABLE_END        = 0x3eff;
    static const int NAMETABLE_MAX_BITS   = 0x2fff;
    static const int PALETTE_RAM_START    = 0x3f00;
    static const int PALETTE_RAM_END      = 0x3fff;
    static const int PALETTE_RAM_MAX_BITS = 0x3f1f;
    static const int PATTERN_TABLE_SIZE = 0x1000;
    static const int NAMETABLE_SIZE     = 0x0400;
private:
    static const int RAMSIZE  = 2048;
    static const int VRAMSIZE = 2048;
    uint16_t truncate_ram_address(uint16_t);
    uint16_t truncate_vram_address(uint16_t);
    uint16_t nametable_mirroring_calculator(uint16_t);
    void process_oam_dma(uint8_t);
    std::array<uint8_t, RAMSIZE>  ram;
    std::array<uint8_t, VRAMSIZE> vram;
public:
    Bus();
    void connect_rom(Rom *);
    void connect_ppu(Ppu *);
    void connect_controller(Controller *);
    uint8_t read_ram(uint16_t);
    uint16_t read_ram_16(uint16_t);
    void write_ram(uint16_t, uint8_t);
    void write_ram_16(uint16_t, uint16_t);
    uint8_t read_vram(uint16_t);
    void write_vram(uint16_t, uint8_t);
    void vram_debug_view(int, int);
    Rom *rom;
    Ppu *ppu;
    Controller *controller;
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