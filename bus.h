#ifndef BUS_H
#define BUS_H
#include <stdint.h>
#define RAMSIZE 0xffff

struct BUS {
    uint8_t ram[RAMSIZE]
};

#endif
