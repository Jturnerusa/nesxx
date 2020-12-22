#ifndef ROM_H
#define ROM_H

#include <cstdint>
#include <vector>

const int HEADER_SIZE = 16;
const int TRAINER_SIZE = 512;
const int PRGROM_UNIT_SIZE = 16384;
const int CHRROM_UNIT_SIZE = 8192;

enum Mapper {
    nrom
};

class Rom{
    public:
        Rom(char*);
        uint8_t read_prgrom(uint16_t);
        uint8_t read_chrrom(uint16_t);
    private:
        Mapper mapper;
        std::vector<uint8_t> rom;
        std::vector<uint8_t> prgrom;
        std::vector<uint8_t> chrrom;
        uint16_t trunacate_prgrom_address(uint16_t);
};

#endif