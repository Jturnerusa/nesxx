#ifndef ROM_H
#define ROM_H

#include <cstdio>
#include <cstdint>
#include <vector>

class Rom{
    public:
        Rom(FILE*);
        std::vector<uint8_t> rom;
        std::vector<uint8_t> prgrom;
        std::vector<uint8_t> chrrom;
};

#endif