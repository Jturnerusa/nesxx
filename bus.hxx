#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>
#include "rom.hxx"

const int RAMSIZE = 2048;
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

class Bus {
    public:
        Bus(Rom*);
        uint8_t read_ram(uint16_t);
        uint16_t read_ram_16(uint16_t);
        void write_ram(uint16_t, uint8_t);
        void write_ram_16(uint16_t, uint16_t);
    private:
        uint16_t trunacte_address(uint16_t);
        Rom *rom;
        std::array<uint8_t, RAMSIZE> ram;
};

void test_bus();

#endif
