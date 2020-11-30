#include <stdint.h>
#ifndef CPU_H
#define CPU_H
#define RAMSIZE 0xffff
#define STACK_OFFSET 0x100

struct CPU
{
    uint16_t pc;
    uint8_t sp;
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t p;
    uint8_t ram[RAMSIZE];
    int page_crossed;
    uint8_t opcode;
	uint16_t opcode_data;
	uint64_t cycles;
};

void init_cpu(struct CPU *cpu);

void run_cpu_tests();

#endif
