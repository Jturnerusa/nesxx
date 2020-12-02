#include "cpu.h"
#include "config.h"

void init_cpu(struct CPU *cpu) {
    cpu->pc = 0;
    cpu->sp = 0xff;
    cpu->a = 0;
    cpu->x = 0;
    cpu->y = 0;
    //From the highest bit to the lowest bit the  flags go
    //Negative, OverFlow, N/A, Decimal, Intterupt, Zero, Carry
    cpu->p = 0;
    for (int x = 0; x < RAMSIZE; x++) {
        cpu->ram[x] = 0;
    }
    cpu->page_crossed = 0;
    cpu->opcode = 0;
	cpu->opcode_data = 0;
	cpu->cycles = 0;
}

/* These functions are for convenience */

int check_if_page_crossed(uint16_t pc, uint16_t address) {
    int pc_page = 1;
    int address_page = 1;
    while (pc >= 256 * pc_page) {
        pc_page++;
    }
    while (address >= 256 * address_page) {
        address_page++;
    }
    if (pc_page != address_page) {
        return 1;
    }
    else {
        return 0;
    }   
}

void set_carry_flag(struct CPU *cpu, int set_on) {
	if (set_on) {
		cpu->p |= 0b1;
	}
	else {
		cpu->p &= 0b11111110;
	}
}	

void set_zero_flag(struct CPU *cpu, int set_on) {
	if (set_on) {
		cpu->p |= 0b10;
	}
	else {
		cpu->p &= 0b11111101;
	}
}

void set_interrupt_flag(struct CPU *cpu, int set_on) {
    if (set_on) {
		cpu->p |= 0b100;
	}
	else {
		cpu->p &= 0b11111011;
	}
}

void set_decimal_flag(struct CPU *cpu, int set_on) {
    if (set_on) {
		cpu->p |= 0b1000;
	}
	else {
		cpu->p &= 0b11110111;
	}
}

void set_break_flag(struct CPU *cpu, int set_on) {
    if (set_on) {
        cpu->p |= 0b10000;
    }
    else {
        cpu->p &= 0b11101111;
    }
}

void set_overflow_flag(struct CPU *cpu, int set_on) {
    if (set_on) {
        cpu->p |= 0b1000000;
    }
    else {
        cpu->p &= 0b10111111;
    }
}

void set_negative_flag(struct CPU *cpu, int set_on) {
    if (set_on) {
        cpu->p |= 0b10000000;
    }
    else {
        cpu->p &= 0b01111111;
    }
}

uint8_t read_carry_flag(struct CPU *cpu) {
	return cpu->p & 0b1;
}

uint8_t read_zero_flag(struct CPU *cpu) {
	return cpu->p & 0b10;
}

uint8_t read_interrupt_flag(struct CPU *cpu) {
	return cpu->p & 0b100;
}

uint8_t read_decimal_flag(struct CPU *cpu) {
	return cpu->p & 0b1000;
}

uint8_t read_break_flag(struct CPU *cpu) {
	return cpu->p & 0b10000;
}

uint8_t read_overflow_flag(struct CPU *cpu) {
	return cpu->p & 0b01000000;
}

uint8_t read_negative_flag(struct CPU *cpu) {
	return cpu->p & 0b10000000;
}

/* We pass a pointer to either the memory address that is going to be operated
 * on or a pointer to the accumulator depending on the CPUs current addressing
 * mode and the opcode. The reason for passing a pointer is so we don't need
 * extra functions for opcodes that can operate on either a memory address or
 * the accumulator, the opcode functions do not know where they are accessing
 * and should not need to know. */

uint8_t *address_accumulator(struct CPU *cpu) {
	return &cpu->a;
}

uint8_t *immediate(struct CPU *cpu) {
    return &cpu->ram[cpu->pc + 1];
}

uint8_t *address_zero_page(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 1] & 0xff;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
	return &cpu->ram[address];  
}

uint8_t *address_zero_page_x(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 1] + cpu->x & 0xff;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
	return &cpu->ram[address];	
}

uint8_t *address_zero_page_y(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 1] + cpu->y & 0xff;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
	return &cpu->ram[address];
}

uint8_t *address_absoulte(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 2] << 8;
    address |= cpu->ram[cpu->pc + 1];
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_absoulte_x(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 2] << 8;
    address |= cpu->ram[cpu->pc + 1];
    address += cpu->x;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_absoulte_y(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 2] << 8;
    address |= cpu->ram[cpu->pc + 1];
    address += cpu->y;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_indirect(struct CPU *cpu) {
    uint16_t absolute_address = cpu->ram[cpu->pc + 1] << 8;
    absolute_address |= cpu->ram[cpu->pc];
    uint16_t address = cpu->ram[absolute_address + 1] << 8;
    address |= cpu->ram[absolute_address];
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_indexed_indirect(struct CPU *cpu) {
    uint8_t zero_page = cpu->ram[cpu->pc + 1];
    zero_page += cpu->x;
    uint16_t address = cpu->ram[zero_page + 1] << 8;
    address |= cpu->ram[zero_page];
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_indirect_indexed(struct CPU *cpu) {
    uint8_t zero_page = cpu->ram[cpu->pc + 1];
	uint16_t address = cpu->ram[zero_page + 1] << 8;
	address |= cpu->ram[zero_page];
	address += cpu->y;
	cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

/*	These functions are used for jump instructions that need the actual
 *	16bit address instead of the value at the address. */

uint16_t address_location_absoulte(struct CPU *cpu) {
	uint16_t address_absoulte = cpu->ram[cpu->pc + 2] << 8;
	address_absoulte |= cpu->ram[cpu->pc + 1];
	return address_absoulte;
}

uint16_t address_location_indirect(struct CPU *cpu) {
	uint16_t absoulte_address = cpu->ram[cpu->pc + 2] << 8;
	absoulte_address |= cpu->ram[cpu->pc + 1];
	uint16_t indirect_location = cpu->ram[absoulte_address + 1] << 8;
	indirect_location |= cpu->ram[absoulte_address];
	return indirect_location;
}

uint8_t pop(struct CPU *cpu) {
    cpu->sp++;
	uint8_t i = cpu->ram[cpu->sp + STACK_OFFSET];
	return i;
}

void push(struct CPU *cpu, uint8_t i) {
	cpu->ram[cpu->sp + STACK_OFFSET] = i;
	cpu->sp--;
}

/* Opcodes */

//Exclusive or accumulator and $address

//Increment $address
void INC(struct CPU *cpu, uint8_t *address) {
	*address++;
	if (*address == 0) {
	    set_zero_flag(cpu, 1);
	}
	if (*address & 0b10000000) {
	    set_negative_flag(cpu, 1);
	}
}

//Increment x
void INX(struct CPU *cpu) {
	cpu->x++;
}

//Increment y
void INY(struct CPU *cpu) {
	cpu->y++;
}

//Jump
void JMP(struct CPU *cpu, uint16_t address_location) {
	cpu->pc = address_location;
}

//Jump to subroutine
void JSR(struct CPU *cpu, uint16_t address_location) {
	push(cpu, cpu->pc - 1);
	cpu->pc = address_location;
}

//Load $address into accumulator
void LDA(struct CPU *cpu, uint8_t *address) {
	cpu->a = *address;
	if (cpu->a == 0) {
		set_zero_flag(cpu, 1);
	}
	if (cpu->a & 0b10000000) {
		set_negative_flag(cpu, 1);
	}
}

//Load $address into x
void LDX(struct CPU *cpu, uint8_t *address) {
	cpu->x = *address;
	if (cpu->x == 0) {
		set_zero_flag(cpu, 1);
	}
	if (cpu->x & 0b10000000) {
		set_negative_flag(cpu, 1);
	}
}

//Load $address into y
void LDY(struct CPU *cpu, uint8_t *address) {
	cpu->y = *address;
	if (cpu->y == 0) {
		set_zero_flag(cpu, 1);
	}
	if (cpu->y & 0b1000000) {
		set_negative_flag(cpu, 1);
	}
}

//Logical shift right
void LSR(struct CPU *cpu, uint8_t *address) {
	uint8_t old_bit_zero = *address & 0b1;
	uint8_t old_bit_seven = *address >> 7;
	*address >>= 1;
	set_carry_flag(cpu, old_bit_zero);
	if (*address == 0) {
		set_zero_flag(cpu, 1);
	}
	if (old_bit_seven) {
		set_negative_flag(cpu, 1);
	}
}

//Logical inclusive OR
void ORA(struct CPU *cpu, uint8_t *address) {
    cpu->a |= *address;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a >> 7) {
        set_negative_flag(cpu, 1);
    }
}

//Push accumulator
void PHA(struct CPU *cpu) {
    push(cpu, cpu->a);
}

//Push processor status
void PHP(struct CPU *cpu) {
    push(cpu, cpu->p);
}

//Pop accumulator
void PLA(struct CPU *cpu) {
    cpu->a = pop(cpu);
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a >> 7) {
        set_negative_flag(cpu, 1);
    }
}

//Pop proccessor status
void PLP(struct CPU *cpu) {
    cpu->p = pop(cpu);
}

//Rotate left
void ROL(struct CPU *cpu, uint8_t *address) {
    uint8_t old_bit_seven = *address >> 7;
    //*address = *address << 1 | (*address & 0b1000000) >> 7;
    *address <<= 1;
    *address |= read_carry_flag(cpu);
    set_carry_flag(cpu, old_bit_seven);
}

//Rotate right
void ROR(struct CPU *cpu, uint8_t *address) {
    uint8_t old_bit_zero = *address & 0b1;
    //*address = (*address >> 1) | (*address & 0b1) << 7;
    *address >>=  1;
    *address |= read_carry_flag(cpu) << 7;
    set_carry_flag(cpu, old_bit_zero);
}

//Return from interrupt
void RTI(struct CPU *cpu) {
	cpu->p = pop(cpu);
	cpu->sp = pop(cpu);
}

//Return from subroutine
void RTS(struct CPU *cpu) {
	cpu->pc = pop(cpu) - 1;
}

//Subtract value of $address + ~carry from accumulator
void SBC(struct CPU *cpu, uint8_t *address) {
    uint8_t difference = (cpu->a - *address) - (~cpu->p & 0b1);
    if ((*address + (~cpu->p & 0b1)) > cpu->a) {
        cpu->p |= 0b1;
    }
    if (difference == 0) {
        set_zero_flag(cpu, 1);
    }
    if ((cpu->a & 0b10000000) != (difference & 0b10000000)) {
        set_overflow_flag(cpu, 1);
    }
    if (difference & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
    cpu->a = difference;
}

//Set carry flag
void SEC(struct CPU *cpu) {
    set_carry_flag(cpu, 1);
}

//Set decimal flag
void SED(struct CPU *cpu) {
    set_decimal_flag(cpu, 1);
}

//Set interrupt disable flag
void SEI(struct CPU *cpu) {
    set_interrupt_flag(cpu, 1);
}

//Store accumulator into $address
void STA(struct CPU *cpu, uint8_t *address) {
    *address = cpu->a;
}

//Store y into $address
void STY(struct CPU *cpu, uint8_t *address) {
    *address = cpu->y;
}

//Transfer accumulator to x
void TAX(struct CPU *cpu) {
    cpu->x = cpu->a;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }  
}

//Transfer accumulator to y
void TAY(struct CPU *cpu) {
    cpu->y = cpu->a;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }  
}

//Transfer stack pointer to x
void TSX(struct CPU *cpu) {
    cpu->x = cpu->sp;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }  
}

//Transfer x to accumulator
void TXA(struct CPU *cpu) {
    cpu->a = cpu->x;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }  
}

//Transfer x to stack pointer
void TXS(struct CPU *cpu) {
    cpu->sp = cpu->x;
}

//Transfer Y to Accumulator
void TYA(struct CPU *cpu) {
    cpu->a = cpu->y;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }  
}

/* Opcode lookup and run */

void run_instruction(struct CPU *cpu) {
    uint8_t opcode = cpu->ram[cpu->pc];
    switch(opcode) {
        case 0x98:
            TYA(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
    }
}

/* Unit tests */
#ifdef TEST

#include <stdio.h>
#include <assert.h>

void test_address_zero_page() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.ram[1] = 0xff;
	cpu.ram[0xff] = 0xee;
    assert(*address_zero_page(&cpu) == 0xee);
}

void test_address_zero_page_x() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.x = 0x01;
    cpu.ram[1] = 0xf0;
    cpu.ram[0xf1] = 0xee;
    assert(*address_zero_page_x(&cpu) == 0xee);
}

void test_address_zero_page_y() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.y = 0x01;
    cpu.ram[1] = 0xf0;
    cpu.ram[0xf1] = 0xee;
    assert(*address_zero_page_y(&cpu) == 0xee);
}

void test_address_absoulte() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf001] = 0xee;
    assert(*address_absoulte(&cpu) == 0xee);
}

void test_address_absoulte_x() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.x = 0x01;
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf002] = 0xee;
    assert(*address_absoulte_x(&cpu) == 0xee);
}

void test_address_absoulte_y() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.y = 0x01;
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf002] = 0xee;
    assert(*address_absoulte_y(&cpu) == 0xee);
}

void test_address_indirect() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.ram[0x01] = 0xf0;
    cpu.ram[0x02] = 0x00;
    cpu.ram[0xf000] = 0xfc;
    cpu.ram[0xf001] = 0xba;
    cpu.ram[0xbafc] = 0xee;
    assert(*address_indirect(&cpu) == 0xee);
}

void test_address_indexed_indirect() {
	struct CPU cpu;
	init_cpu(&cpu);
	cpu.ram[1] = 0x03;
	cpu.x = 0x01;
	cpu.ram[0x04] = 0x05;
	cpu.ram[0x05] = 0x07;
	cpu.ram[0x0705] = 0xee;
	assert(*address_indexed_indirect(&cpu) == 0xee);
}

void test_address_indirect_indexed() {
	struct CPU cpu;
	init_cpu(&cpu);
	cpu.ram[1] = 0x03;
	cpu.y = 0x01;
	cpu.ram[0x03] = 0x05;
	cpu.ram[0x04] = 0x07;
	cpu.ram[0x0706] = 0xee;
	assert(*address_indirect_indexed(&cpu) == 0xee);
}

void test_page_crossed() {
	assert(check_if_page_crossed(255,256) == 1);
	assert(check_if_page_crossed(200,300) == 1);
	assert(check_if_page_crossed(1, 1) == 0);
}

void test_SBC() {
    struct CPU cpu;
    //Test SBC w/ carry flag set
    init_cpu(&cpu);
    cpu.a = 5;
    cpu.ram[0] = 3;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
    assert(cpu.a == 2);
    //Test SBC w/o carry flag set
    init_cpu(&cpu);
    cpu.a = 5;
    cpu.ram[0] = 3;
    SBC(&cpu, &cpu.ram[0]);
    assert(cpu.a == 1);
    //Test if SBC sets negative flag
    init_cpu(&cpu);
    cpu.a = 5;
    cpu.ram[0] = 5;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
    assert(cpu.p & 0b10);
    //Test if SBC sets carry flag
    init_cpu(&cpu);
    cpu.a = 5;
    cpu.ram[0] = 6;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
    assert(cpu.p & 0b1);
    //Test if SBC sets overflow flag
    init_cpu(&cpu);
    cpu.a = 46;
    cpu.ram[0] = 64;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
    assert(cpu.p & 0b01000000);
}

void test_pop_push() {
    struct CPU cpu;
    init_cpu(&cpu);
    uint8_t value = 1;
    push(&cpu, value);
    value = 2;
    push(&cpu, value);
    assert(pop(&cpu) == 2);
    assert(pop(&cpu) == 1);
}

void test_ROR() {
    struct CPU cpu;
    init_cpu(&cpu);
    //Test rotation
    set_carry_flag(&cpu, 1);
    cpu.ram[0xf] = 0b10000011;
    ROR(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b11000001);
    ROR(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b11100000);
    //Test carry flag, new bit seven should == current carry flag
    //and carry flag should get set to old bit zero.
    init_cpu(&cpu);
    cpu.ram[0xf] = 0b00000011;
    ROR(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b00000001);
    assert(read_carry_flag(&cpu) == 1);
}

void test_ROL() {
    struct CPU cpu;
    init_cpu(&cpu);
    //Test rotation
    set_carry_flag(&cpu, 1);
    cpu.ram[0xf] = 0b11000000;
    ROL(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b10000001);
    ROL(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b00000011);
    //Test carry flag, new bit zero == current carry flag
    //then carry flag == old bit seven.
    init_cpu(&cpu);
    cpu.ram[0xf] = 0b11000000;
    ROL(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b10000000);
    set_carry_flag(&cpu, 1);
    cpu.ram[0xf] = 0b01111111;
    ROL(&cpu, &cpu.ram[0xf]);
    assert(read_carry_flag(&cpu) == 0);
}

void run_cpu_tests() {
    printf("Testing addressing mode zero page\n");
    test_address_zero_page();
    printf("Testing addressing mode zero page x\n");
    test_address_zero_page_x();
    printf("Testing addressing mode zero page y\n");
    test_address_zero_page_y();
    printf("Testing addressing mode absoulte\n");
    test_address_absoulte();
    printf("Testing addressing mode absoulte x\n");
    test_address_absoulte_x();
    printf("Testing addressing mode absoulte y\n");
    test_address_absoulte_y();
    printf("Testing addressing mode indirect\n");
    test_address_indirect();
    printf("Testing addressing mode indexed indirect\n");
	test_address_indexed_indirect();
	printf("Testing addressing mode indirect indexed\n");
    test_address_indirect_indexed();
    printf("Testing page crossed function\n");
	test_page_crossed();
    printf("Testing stack pop/push\n");
	test_pop_push();
	printf("Testing opcode SBC\n");
	test_SBC();
	printf("Testing opcode ROR\n");
	test_ROR();
	printf("Testing opcode ROL\n");
	test_ROL();
}
#endif

