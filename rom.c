#include <stdio.h>
#include "rom.h"

void init_rom(struct ROM *rom, char *rom_file_data, size_t rom_file_size) {
    if (rom_file_size < 16) {
        printf("Rom less than 16 bytes\n");
        exit(EXIT_FAILURE);
    }
    uint8_t rom_header[ROM_HEADER_SIZE];
    for (int i = 0; i < ROM_HEADER_SIZE; i++) {
        rom_header[i] = (uint8_t) rom_file_data[i];
    }
    rom->trainer_present = rom_header[6] & 0b100 >> 2;
    rom->prgrom_size = rom_header[4] * PRGROM_UNIT_SIZE;
    rom->chrrom_size = rom_header[5] * CHRROM_UNIT_SIZE;
    rom->prgrom_data = malloc(sizeof(uint8_t) * rom->prgrom_size);
    rom->chrrom_data = malloc(sizeof(uint8_t) * rom->chrrom_size);
    int prgrom_offset = (TRAINER_SIZE * rom->trainer_present) + 16;
    int chrrom_offset = prgrom_offset + rom->prgrom_size;
    if (rom_file_size < chrrom_offset + rom->chrrom_size) {
        printf("Rom not long!\n");
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rom->prgrom_size; i++) {
        rom->prgrom_data[i] = rom_file_data[prgrom_offset + i];
    }
    for (int i = 0; i < rom->chrrom_size; i++) { 
        rom->chrrom_data[i] = rom_file_data[chrrom_offset + i];
    }
}
