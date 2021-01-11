#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>

class Rom;
class Ppu;

const int RAMSIZE = 2048;
const int VRAMSIZE = 2048;

const int RAM_ADDRESS_START = 0;
const int RAM_ADDRESS_END = 0x1fff;
const int RAM_ADDRESS_MAX_BITS = 0x07ff;
const int PPU_IO_ADDRESS_START = 0x2000;
const int PPU_IO_ADDRESS_END = 0x3fff;
const int PPU_IO_ADDRESS_MAX_BITS = 0x2007;
const int APU_IO_ADDRESS_START = 0x4000;
const int APU_IO_ADDRESS_END = 0x4017;
const int APU_IO_DISABLED_START = 0x4018;
const int APU_IO_DISABED_END = 0x401f;
const int ROM_IO_START = 0x8000;
const int ROM_IO_END = 0xffff;

const int PATTERN_TABLE_START = 0x0000;
const int PATTERN_TABLE_END = 0x1fff;
const int NAMETABLE_START = 0x2000;
const int NAMETABLE_END = 0x3eff;
const int NAMETABLE_MAX_BITS = 0x2fff;
const int PALETTE_RAM_START = 0x3f00;
const int PALETTE_RAM_END = 0x3fff;
const int PALETTE_RAM_MAX_BITS = 0x3f1f;

class Bus {
public:
    void connect_rom(Rom*);
    void connect_ppu(Ppu*);
    uint8_t read_ram(uint16_t);
    uint16_t read_ram_16(uint16_t);
    void write_ram(uint16_t, uint8_t);
    void write_ram_16(uint16_t, uint16_t);
    uint8_t read_vram(uint16_t);
    void write_vram(uint16_t, uint8_t);
    void vram_debug_view(int, int);
    Rom *rom;
    Ppu *ppu;
private:
    uint16_t truncate_ram_address(uint16_t);
    uint16_t truncate_vram_address(uint16_t);
    uint16_t nametable_mirroring_calculator(uint16_t);
    std::array<uint8_t, RAMSIZE> ram;
    std::array<uint8_t, VRAMSIZE> vram;
};

void test_bus();

#endif
