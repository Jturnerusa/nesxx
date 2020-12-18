#ifndef BUS_H
#define BUS_H

#include <cstdint>
#include <array>

const int RAMSIZE = 2048;

class Bus {
    public:
        Bus();
        uint8_t read_ram(uint16_t);
        uint16_t read_ram_16(uint16_t);
        void write_ram(uint16_t, uint8_t);
        void write_ram_16(uint16_t, uint16_t);
    private:
        std::array<uint8_t, RAMSIZE> ram;
};

#endif
