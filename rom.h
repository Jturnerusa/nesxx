#ifndef ROM_H
#define ROM_H
#include <stdlib.h>
#include <stdint.h>
#include "config.h"
#define ROM_HEADER_SIZE 16
#define TRAINER_SIZE 512
#define PRGROM_UNIT_SIZE 16384
#define CHRROM_UNIT_SIZE 8192


struct ROM {
    size_t prgrom_size;
    size_t chrrom_size;
    uint8_t *prgrom_data;
    uint8_t *chrrom_data;
    int trainer_present;
};

void init_rom(struct ROM *rom, char *rom_file_data, size_t rom_file_length);

#endif
