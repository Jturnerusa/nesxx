#ifndef CPU_H
#include <stdint.h>
#include "rom.h"
#include "config.h"
#define CPU_H
#define RAMSIZE 0xffff
#define STACK_OFFSET 0x100
#define PRGROM_OFFSET 0xc000

struct CPU
{
    uint16_t pc;
    uint8_t sp;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t ram[RAMSIZE];
    uint8_t opcode;
    uint16_t opcode_data;
    int page_crossed;
    uint64_t iterations;
	uint64_t cycles;
	uint16_t ppu_io_accessed;   //This will store the addr of the accessed mem mapped area
	int ppu_io_was_read;         //This will let us know whether the ppu io was a read
	int ppu_io_was_write;        //and this will let us know whether it was a write
	int interrupt_nmi;
};

void init_cpu(struct CPU *self);

void load_prgrom(struct CPU *self, struct ROM *rom);

void run_instruction(struct CPU *self);

#ifdef UNITTEST
void run_cpu_tests();
#endif

#endif
