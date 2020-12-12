#include <stdlib.h>
#include <stdio.h>
#include "cpu.h"

void init_cpu(struct CPU *self) {
    self->pc = 0xc000;
    self->sp = 0xfd;
    self->a = 0;
    self->x = 0;
    self->y = 0;
    //From the highest bit to the lowest bit the  flags go
    //Negative, OverFlow, N/A, Decimal, Intterupt, Zero, Carry
    self->p = 0x24;
    for (int x = 0; x < RAMSIZE; x++) {
        self->ram[x] = 0;
    }
    self->page_crossed = 0;
    self->opcode = 0;
	self->opcode_data = 0;
	self->iterations = 0;
	self->cycles = 0;
}

void load_prgrom(struct CPU *self, struct ROM *rom) {
    if (rom->prgrom_size + PRGROM_OFFSET > RAMSIZE + 1) {
        printf("Rom will not fit into CPU's ram! %d\n", rom->prgrom_size);
        exit(EXIT_FAILURE);
    }
    for (int i = 0; i < rom->prgrom_size; i++) {
        self->ram[PRGROM_OFFSET + i] = rom->prgrom_data[i];
    }
}

//Emulating memory mirroring by disgarding some of the higher bits of the requested address!
uint16_t trunacate_address(uint16_t address) {
    if (address <= 0x1fff) {
        return address & 0x07ff;
    }
    if (address > 0x1fff & address <= 0x3fff) {
        return address & 0x2007;
    }
    return address;
}

//Instead of having a function to read and write ram, this just returns a pointer to the ram address for
//reading and writing. Not sure if this is the best design choice but it would take a lot of work to refactor.
//:-/
uint8_t *access_ram(struct CPU *self, uint16_t address) {
    uint16_t trunacated_address = trunacate_address(address);
    return &self->ram[trunacated_address];
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

void set_carry_flag(struct CPU *self, int set_on) {
	if (set_on) {
		self->p |= 0b1;
	}
	else {
		self->p &= 0b11111110;
	}
}	

void set_zero_flag(struct CPU *self, int set_on) {
	if (set_on) {
		self->p |= 0b10;
	}
	else {
		self->p &= 0b11111101;
	}
}

void set_interrupt_flag(struct CPU *self, int set_on) {
    if (set_on) {
		self->p |= 0b100;
	}
	else {
		self->p &= 0b11111011;
	}
}

void set_decimal_flag(struct CPU *self, int set_on) {
    if (set_on) {
		self->p |= 0b1000;
	}
	else {
		self->p &= 0b11110111;
	}
}

void set_break_flag(struct CPU *self, int set_on) {
    if (set_on) {
        self->p |= 0b10000;
    }
    else {
        self->p &= 0b11101111;
    }
}

void set_overflow_flag(struct CPU *self, int set_on) {
    if (set_on) {
        self->p |= 0b1000000;
    }
    else {
        self->p &= 0b10111111;
    }
}

void set_negative_flag(struct CPU *self, int set_on) {
    if (set_on) {
        self->p |= 0b10000000;
    }
    else {
        self->p &= 0b01111111;
    }
}

uint8_t read_carry_flag(struct CPU *self) {
	if (self->p & 0b1) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_zero_flag(struct CPU *self) {
	if (self->p & 0b10) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_interrupt_flag(struct CPU *self) {
	if (self->p & 0b100) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_decimal_flag(struct CPU *self) {
	if (self->p & 0b1000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_break_flag(struct CPU *self) {
	if (self->p & 0b10000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_overflow_flag(struct CPU *self) {
	if (self->p & 0b01000000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_negative_flag(struct CPU *self) {
	if (self->p & 0b10000000) {
		return 1;
	}
	else {
		return 0;
	}
}

/* We pass a pointer to either the memory address that is going to be operated
 * on or a pointer to the accumulator depending on the CPUs current addressing
 * mode and the opcode. The reason for passing a pointer is so we don't need
 * extra functions for opcodes that can operate on either a memory address or
 * the accumulator, the opcode functions do not know where they are accessing
 * and should not need to know. */

uint8_t *address_accumulator(struct CPU *self) {
	return &self->a;
}

uint8_t *address_immediate(struct CPU *self) {
    return access_ram(self, self->pc + 1);
}

uint8_t *address_zero_page(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 1);
    self->page_crossed = check_if_page_crossed(self->pc, address);
	return access_ram(self, address);  
}

uint8_t *address_zero_page_x(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 1) + self->x & 0xff;
    self->page_crossed = check_if_page_crossed(self->pc, address);
	return access_ram(self, address);	
}

uint8_t *address_zero_page_y(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 1) + self->y & 0xff;
    self->page_crossed = check_if_page_crossed(self->pc, address);
	return access_ram(self, address);
}

uint8_t *address_relative(struct CPU *self) {
    return access_ram(self, self->pc + 1);
}

uint8_t *address_absolute(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 2) << 8;
    address |= *access_ram(self, self->pc + 1);
    self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

uint8_t *address_absolute_x(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 2) << 8;
    address |= *access_ram(self, self->pc + 1);
    address += self->x;
    self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

uint8_t *address_absolute_y(struct CPU *self) {
    uint16_t address = *access_ram(self, self->pc + 2) << 8;
    address |= *access_ram(self, self->pc + 1);
    address += self->y;
    self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

uint8_t *address_indirect(struct CPU *self) {
    uint16_t absolute_address = *access_ram(self, self->pc + 1) << 8;
    absolute_address |= *access_ram(self, self->pc);
    uint16_t address = *access_ram(self, absolute_address + 1) << 8;
    address |= *access_ram(self, absolute_address);
    self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

uint8_t *address_indexed_indirect(struct CPU *self) {
    uint8_t zero_page = *access_ram(self, self->pc + 1);
    zero_page += self->x;
    uint16_t address = *access_ram(self, (uint8_t) (zero_page + 1)) << 8;
    address |= *access_ram(self, zero_page);
    self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

uint8_t *address_indirect_indexed(struct CPU *self) {
    uint8_t zero_page = *access_ram(self, self->pc + 1);
	uint16_t address = *access_ram(self, (uint8_t) (zero_page + 1)) << 8;
	address |= *access_ram(self, zero_page);
	address += self->y;
	self->page_crossed = check_if_page_crossed(self->pc, address);
    return access_ram(self, address);
}

/*	These functions are used for jump instructions that need the actual
 *	16bit address instead of the value at the address. */

uint16_t address_location_absolute(struct CPU *self) {
	uint16_t address_absolute = *access_ram(self, self->pc + 2) << 8;
	address_absolute |= *access_ram(self, self->pc + 1);
	return address_absolute;
}

uint16_t address_location_indirect(struct CPU *self) {
    //This needs to wrap around to the bottom of the current page if the absoulte jump location is at the end
    //of the page. In other words 0x1ff wraps around to 0x100 rather than going 0x200
    uint16_t absoulte_a = (*access_ram(self, self->pc + 2) << 8) | *access_ram(self, self->pc + 1);
    uint16_t absoulte_b = (*access_ram(self, self->pc + 2) << 8) | (uint8_t)(*access_ram(self, self->pc + 1) + 1);
    uint16_t indirect_address = (*access_ram(self, absoulte_b) << 8) | *access_ram(self, absoulte_a);
    return indirect_address;
}

/* Basic stack functions */

uint8_t pop(struct CPU *self) {
    self->sp++;
	uint8_t i = *access_ram(self, self->sp + STACK_OFFSET);
	return i;
}

void push(struct CPU *self, uint8_t i) {
    printf("pushing %x\n", i);
	*access_ram(self, self->sp + STACK_OFFSET) = i;
	self->sp--;
}

/* Opcodes */

//Add with carry
void ADC(struct CPU *self, uint8_t *address) {
	unsigned int sum = self->a + *address + read_carry_flag(self);
	set_carry_flag(self, sum > 0xff);
    set_overflow_flag(self, (self->a ^ sum) & (*address ^ sum) & 0x80);
	self->a = sum;
	set_zero_flag(self, self->a == 0);
	set_negative_flag(self, self->a >> 7);
}

//Logical and accumulator & $address
void AND(struct CPU *self, uint8_t *address) {
	self->a &= *address;
	set_zero_flag(self, self->a == 0);
	set_negative_flag(self, self->a & 0b10000000);
	
}

//Arithmetic shift left
void ASL(struct CPU *self, uint8_t *address) {
    uint8_t old_bit_seven = *address >> 7;
    *address <<= 1;
    set_carry_flag(self, old_bit_seven);
    set_zero_flag(self, *address == 0);
    set_negative_flag(self, *address >> 7);
}

//Branch if carry flag is not set
void BCC(struct CPU *self, uint8_t *address) {
    if (read_carry_flag(self) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Branch if carry flag is set
void BCS(struct CPU *self, uint8_t *address) {
    if (read_carry_flag(self)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Branch if zero flag is set
void BEQ(struct CPU *self, uint8_t *address) {
    if (read_zero_flag(self)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Bit test
void BIT(struct CPU *self, uint8_t *address) {
    uint8_t result = *address & self->a;
    set_zero_flag(self, result == 0);
    set_overflow_flag(self, *address & 0b01000000);
    set_negative_flag(self, *address & 0b10000000);
}

//Branch if negative flag is set
void BMI(struct CPU *self, uint8_t *address){
    if (read_negative_flag(self)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Branch if zero flag is not set
void BNE(struct CPU *self, uint8_t *address){
    if (read_zero_flag(self) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Branch if negative flag is not set
void BPL(struct CPU *self, uint8_t *address){
    if (read_negative_flag(self) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//TODO Force interrupt
//void BRK(struct CPU *self) {
//}

//Branch if overflow not set
void BVC(struct CPU *self, uint8_t *address) {
    if (read_overflow_flag(self) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Branch if overflow flag is set
void BVS(struct CPU *self, uint8_t *address) {
    if (read_overflow_flag(self)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(self->pc, self->pc + offset)) {
            self->page_crossed = 1;
        }
        self->pc += offset;
    }
}

//Clear carry flag
void CLC(struct CPU *self) {
    set_carry_flag(self, 0);
}

//Clear decimal flag
void CLD(struct CPU *self) {
    set_decimal_flag(self, 0);
}

//Clear interrupt flag
void CLI(struct CPU *self) {
    set_interrupt_flag(self, 0);
}

//Clear overflow flag
void CLV(struct CPU *self) {
    set_overflow_flag(self, 0);
}

//Compare accumulator $address
void CMP(struct CPU *self, uint8_t *address) {
    set_carry_flag(self, self->a >= *address);
    set_zero_flag(self, self->a == *address);
    set_negative_flag(self, (self->a - *address) & 0b10000000);
}

//Compare X $address
void CPX(struct CPU *self, uint8_t *address) {
    set_carry_flag(self, self->x >= *address);
    set_zero_flag(self, self->x == *address);
    set_negative_flag(self, (self->x - *address ) & 0b10000000);
}

//Compare Y $address
void CPY(struct CPU *self, uint8_t *address) {
    set_carry_flag(self, self->y >= *address);
    set_zero_flag(self, self->y == *address);
    set_negative_flag(self, (self->y - *address ) & 0b10000000);
}

//Decrement $address
void DEC(struct CPU *self, uint8_t *address) {
    (*address)--;
    set_zero_flag(self, (*address) == 0);
    set_negative_flag(self, (*address) & 0b10000000);
}

//Decrement X
void DEX(struct CPU *self) {
    self->x--;
    set_zero_flag(self, self->x == 0);
    set_negative_flag(self, self->x & 0b10000000);
}

//Decrement Y
void DEY(struct CPU *self) {
    self->y--;
    set_zero_flag(self, self->y == 0);
    set_negative_flag(self, self->y & 0b10000000);
}

//Exclusive or accumulator and $address
void EOR(struct CPU *self, uint8_t *address)
{
    self->a ^= *address;   
    set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a & 0b10000000);
}

//Increment $address
void INC(struct CPU *self, uint8_t *address) {
	(*address)++;
	set_zero_flag(self, (*address) == 0);
	set_negative_flag(self, (*address) & 0b10000000);
}

//Increment x
void INX(struct CPU *self) {
	self->x++;
    set_zero_flag(self, self->x == 0);
	set_negative_flag(self, self->x & 0b10000000);
}

//Increment y
void INY(struct CPU *self) {
	self->y++;
	set_zero_flag(self, self->y == 0);
	set_negative_flag(self, self->y & 0b10000000);
}

//Jump
void JMP(struct CPU *self, uint16_t address_location) {
	self->pc = address_location;
}

//Jump to subroutine
void JSR(struct CPU *self, uint16_t address_location) {
	push(self, (self->pc + 2) >> 8);
	push(self, (self->pc + 2) & 0xff);
	self->pc = address_location;
}

//Load $address into accumulator
void LDA(struct CPU *self, uint8_t *address) {
	self->a = *address;
	set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a & 0b10000000);

}

//Load $address into x
void LDX(struct CPU *self, uint8_t *address) {
	self->x = *address;
	set_zero_flag(self, self->x == 0);
	set_negative_flag(self, self->x & 0b10000000);
}

//Load $address into y
void LDY(struct CPU *self, uint8_t *address) {
	self->y = *address;
    set_zero_flag(self, self->y == 0);
	set_negative_flag(self, self->y & 0b10000000);
}

//Logical shift right
void LSR(struct CPU *self, uint8_t *address) {
	uint8_t old_bit_zero = *address & 0b1;
	*address >>= 1;
	set_carry_flag(self, old_bit_zero);
    set_zero_flag(self, *address == 0);
	set_negative_flag(self, 0);
}

//Logical inclusive OR
void ORA(struct CPU *self, uint8_t *address) {
    self->a |= *address;
    set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a >> 7);
}

//Push accumulator
void PHA(struct CPU *self) {
    push(self, self->a);
}

//Push processor status
void PHP(struct CPU *self) {
    //The 4th bit needs to be set when pushing for some reason
    push(self, self->p | 0b00010000);
}

//Pop accumulator
void PLA(struct CPU *self) {
    self->a = pop(self);
    set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a >> 7);
}

//Pop proccessor status
void PLP(struct CPU *self) {
    //The 4th bit needs to be unset when popping
    //The 5th bit also needs to be set
    self->p = (pop(self) & 0b11101111)  | 0b00100000;
}

//Rotate left
void ROL(struct CPU *self, uint8_t *address) {
    uint8_t old_bit_seven = *address >> 7;
    //*address = *address << 1 | (*address & 0b1000000) >> 7;
    *address <<= 1;
    *address |= read_carry_flag(self);
    set_carry_flag(self, old_bit_seven);
    set_zero_flag(self, *address == 0);
    set_negative_flag(self, *address & 0b10000000);
}

//Rotate right
void ROR(struct CPU *self, uint8_t *address) {
    uint8_t old_bit_zero = *address & 0b1;
    //*address = (*address >> 1) | (*address & 0b1) << 7;
    *address >>=  1;
    *address |= read_carry_flag(self) << 7;
    set_carry_flag(self, old_bit_zero);
    set_zero_flag(self, *address == 0);
    set_negative_flag(self, *address & 0b10000000);
}

//Return from interrupt
void RTI(struct CPU *self) {
	self->p = pop(self) & 0xef | 0x20;
	self->pc = pop(self);
	self->pc |= pop(self) << 8;
}

//Return from subroutine
void RTS(struct CPU *self) {
    self->pc = pop(self);
    self->pc |= pop(self) << 8;
}

//Subtract value of $address + ~carry from accumulator
void SBC(struct CPU *self, uint8_t *address) {
	*address = ~*address;
	ADC(self, address);
	*address = ~*address;
}

//Set carry flag
void SEC(struct CPU *self) {
    set_carry_flag(self, 1);
}

//Set decimal flag
void SED(struct CPU *self) {
    set_decimal_flag(self, 1);
}

//Set interrupt disable flag
void SEI(struct CPU *self) {
    set_interrupt_flag(self, 1);
}

//Store accumulator into $address
void STA(struct CPU *self, uint8_t *address) {
    *address = self->a;
}

//Store y into $address
void STY(struct CPU *self, uint8_t *address) {
    *address = self->y;
}

//Store x into $address
void STX(struct CPU *self, uint8_t *address) {
    *address = self->x;
}

//Transfer accumulator to x
void TAX(struct CPU *self) {
    self->x = self->a;
    set_zero_flag(self, self->x == 0);
    set_negative_flag(self, self->x & 0b10000000);
}

//Transfer accumulator to y
void TAY(struct CPU *self) {
    self->y = self->a;
    set_zero_flag(self, self->y == 0);
    set_negative_flag(self, self->y & 0b10000000);
}

//Transfer stack pointer to x
void TSX(struct CPU *self) {
    self->x = self->sp;
    set_zero_flag(self, self->x == 0);
    set_negative_flag(self, self->x & 0b10000000);
}

//Transfer x to accumulator
void TXA(struct CPU *self) {
    self->a = self->x;
    set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a & 0b10000000);
}

//Transfer x to stack pointer
void TXS(struct CPU *self) {
    self->sp = self->x;
}

//Transfer Y to Accumulator
void TYA(struct CPU *self) {
    self->a = self->y;
    set_zero_flag(self, self->a == 0);
    set_negative_flag(self, self->a & 0b10000000);
}

/* Debug output */

#if DEBUG==1
#include "cpu_debug.h"
#endif

/* Opcode lookup and run */

void run_instruction(struct CPU *self) {
	self->page_crossed = 0;
    self->opcode = *access_ram(self, self->pc);
    #if DEBUG==1
    print_debug_info(self);
    printf("%x\n", self->ram[0x89]);
    debug_nestest_log_compare(self);
    #endif
    switch(self->opcode) {
        //ADC
        case 0x69:
            ADC(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;      
        case 0x65:
            ADC(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x75:
            ADC(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x6d:
            ADC(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0x7d:
            ADC(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x79:
            ADC(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x61:
            ADC(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x71:
            ADC(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 5;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //AND
        case 0x29:
            AND(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0x25:
            AND(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x35:
            AND(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x2d:
            AND(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0x3d:
            AND(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x39:
            AND(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x21:
            AND(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x31:
            AND(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 5;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //ASL 
        case 0x0a:
            ASL(self, address_accumulator(self));
            self->pc += 1;
            self->cycles += 2;
            break;
        case 0x06:
            ASL(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 5;
            break;
        case 0x16:
            ASL(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x0e:
            ASL(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 6;
            break;
        case 0x1e:
            ASL(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 7;
            break;
        //BCC
        case 0x90:
            BCC(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BCS
        case 0xb0:
            BCS(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BEQ
        case 0xf0:
            BEQ(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BIT
        case 0x24:
            BIT(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x2c:
            BIT(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        //BMI
        case 0x30:
            BMI(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BNE
        case 0xd0:
            BNE(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BPL
        case 0x10:
            BPL(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BVC
        case 0x50:
            BVC(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //BVS
        case 0x70:
            BVS(self, address_relative(self));
            self->pc += 2;
            self->cycles += 2;
            if (self->page_crossed) {
                self->cycles += 2;
            }
            break;
        //CLC
        case 0x18:
            CLC(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //CLD
        case 0xd8:
            CLD(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //CLI
        case 0x58:
            CLI(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //CLV
        case 0xb8:
            CLV(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //CMP
        case 0xc9:
            CMP(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0xc5:
            CMP(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0xd5:
            CMP(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0xcd:
            CMP(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0xdd:
            CMP(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0xd9:
            CMP(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
    	case 0xc1:
			CMP(self, address_indexed_indirect(self));
			self->pc += 2;
			self->cycles += 6;
			break;
		case 0xd1:
			CMP(self, address_indirect_indexed(self));
			self->pc += 2;
			self->cycles += 5;
			if (self->page_crossed) {
				self->cycles += 1;
			}
			break;
		//CPX
		case 0xe0:
			CPX(self, address_immediate(self));
			self->pc += 2;
			self->cycles += 2;
			break;
		case 0xe4:
			CPX(self, address_zero_page(self));
			self->pc += 2;
			self->cycles += 3;
			break;
		case 0xec:
			CPX(self, address_absolute(self));
			self->pc += 3;
			self->cycles += 4;
			break;
		//CPY
		case 0xc0:
			CPY(self, address_immediate(self));
			self->pc += 2;
			self->cycles += 2;
			break;
		case 0xc4:
			CPY(self, address_zero_page(self));
			self->pc += 2;
			self->cycles += 3;
			break;
		case 0xcc:
			CPY(self, address_absolute(self));
			self->pc += 3;
			self->cycles += 4;
			break;
		//DEC
		case 0xc6:
			DEC(self, address_zero_page(self));
			self->pc += 2;
			self->cycles += 5;
			break;
		case 0xd6:
			DEC(self, address_zero_page_x(self));
			self->pc += 2;
			self->cycles += 6;
			break;
		case 0xce:
			DEC(self, address_absolute(self));
			self->pc += 3;
			self->cycles += 6;
			break;
		case 0xde:
			DEC(self, address_absolute_x(self));
			self->pc += 3;
			self->cycles += 7;
			break;
		//DEX
		case 0xca:
			DEX(self);
			self->pc += 1;
			self->cycles += 2;
			break;
		//DEY
		case 0x88:
			DEY(self);
			self->pc += 1;
			self->cycles += 2;
			break;
		//EOR
		case 0x49:
			EOR(self, address_immediate(self));
			self->pc += 2;
			self->cycles += 2;
			break;
		case 0x45:
			EOR(self, address_zero_page(self));
			self->pc += 2;
			self->cycles += 3;
			break;
		case 0x55:
			EOR(self, address_zero_page_x(self));
			self->pc += 2;
			self->cycles += 4;
			break;
		case 0x4d:
			EOR(self, address_absolute(self));
			self->pc += 3;
			self->cycles += 4;
			break;
		case 0x5d:
			EOR(self, address_absolute_x(self));
			self->pc += 3;
			self->cycles += 4;
			if (self->page_crossed) {
				self->cycles += 1;
			}
			break;
		case 0x59:
			EOR(self, address_absolute_y(self));
			self->pc += 3;
			self->cycles += 4;
			if (self->page_crossed) {
				self->cycles += 1;
			}
			break;
		case 0x41:
			EOR(self, address_indexed_indirect(self));
			self->pc += 2;
			self->cycles += 6;
			break;
		case 0x51:
			EOR(self, address_indirect_indexed(self));
			self->pc += 2;
			self->cycles += 5;
			if (self->page_crossed) {
				self->cycles += 1;
			}
			break;
		//INC
		case 0xe6:
			INC(self, address_zero_page(self));
			self->pc += 2;
			self->cycles += 5;
			break;
		case 0xf6:
			INC(self, address_zero_page_x(self));
			self->pc += 2;
			self->cycles += 6;
			break;
		case 0xee:
			INC(self, address_absolute(self));
			self->pc += 3;
			self->cycles += 6;
			break;
		case 0xfe:
			INC(self, address_absolute_x(self));
			self->pc += 3;
			self->cycles += 7;
			break;
		//INX
		case 0xe8:
			INX(self);
			self->pc += 1;
			self->cycles += 2;
			break;
		//INY
		case 0xc8:
			INY(self);
			self->pc += 1;
			self->cycles += 2;
			break;
		//JMP
		case 0x4c:
			JMP(self, address_location_absolute(self));
			self->cycles += 3;
			break;
		case 0x6c:
			JMP(self, address_location_indirect(self));
			self->cycles += 5;
			break;
		//JSR
		case 0x20:
			JSR(self, address_location_absolute(self));
			self->cycles += 6;
			break;
		//LDA
		case 0xa9:
		    LDA(self, address_immediate(self));
		    self->pc += 2;
		    self->cycles += 3;
		    break;
		case 0xa5:
		    LDA(self, address_zero_page(self));
		    self->pc += 2;
		    self->cycles += 3;
		    break;
        case 0xb5:
            LDA(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0xad:
            LDA(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0xbd:
            LDA(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0xb9:
            LDA(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0xa1:
            LDA(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0xb1:
            LDA(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 5;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //LDX
        case 0xa2:
            LDX(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0xa6:
            LDX(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0xb6:
            LDX(self, address_zero_page_y(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0xae:
            LDX(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0xbe:
            LDX(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //LDY
        case 0xa0:
            LDY(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0xa4:
            LDY(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0xb4:
            LDY(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0xac:
            LDY(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0xbc:
            LDY(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //LSR
        case 0x4a:
            LSR(self, address_accumulator(self));
            self->pc += 1;
            self->cycles += 2;
            break;
        case 0x46:
            LSR(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 5;
            break;
        case 0x56:
            LSR(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x4e:
            LSR(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 6;
            break;
        case 0x5e:
            LSR(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 7;
            break;
        //NOP
        case 0xea:
            self->pc += 1;
            self->cycles += 2;
            break;
        case 0x04:
            self->pc += 2;
            self->cycles += 3;
            break;
        //ORA
        case 0x09:
            ORA(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0x05:
            ORA(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x15:
            ORA(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x0d:
            ORA(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0x1d:
            ORA(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x19:
            ORA(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0x01:
            ORA(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x11:
            ORA(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 5;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //PHA
        case 0x48:
            PHA(self);
            self->pc += 1;
            self->cycles += 3;
            break;
        //PHP
        case 0x08:
            PHP(self);
            self->pc += 1;
            self->cycles += 3;
            break;
        //PLA
        case 0x68:
            PLA(self);
            self->pc += 1;
            self->cycles += 4;
            break;
        //PLP
        case 0x28:
            PLP(self);
            self->pc += 1;
            self->cycles += 4;
            break;
        //ROL
        case 0x2a:
            ROL(self, address_accumulator(self));
            self->pc += 1;
            self->cycles += 2;
            break;
        case 0x26:
            ROL(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 5;
            break;
        case 0x36:
            ROL(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x2e:
            ROL(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 6;
            break;
        case 0x3e:
            ROL(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 7;
            break;
        //ROR
        case 0x6a:
            ROR(self, address_accumulator(self));
            self->pc += 1;
            self->cycles += 2;
            break;
        case 0x66:
            ROR(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 5;
            break;
        case 0x76:
            ROR(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x6e:
            ROR(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 6;
            break;
        case 0x7e:
            ROR(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 7;
            break;
        //RTI
        case 0x40:
            RTI(self);
            self->cycles += 6;
            break;
        //RTS
        case 0x60:
            RTS(self);
            self->pc += 1;
            self->cycles += 6;
            break;
        //SBC
        case 0xe9:
            SBC(self, address_immediate(self));
            self->pc += 2;
            self->cycles += 2;
            break;
        case 0xe5:
            SBC(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0xf5:
            SBC(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0xed:
            SBC(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0xfd:
            SBC(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0xf9:
            SBC(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 4;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        case 0xe1:
            SBC(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0xf1:
            SBC(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 5;
            if (self->page_crossed) {
                self->cycles += 1;
            }
            break;
        //SEC
        case 0x38:
            SEC(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //SED
        case 0xf8:
            SED(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //SEI
        case 0x78:
            SEI(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //STA
        case 0x85:
            STA(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x95:
            STA(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x8d:
            STA(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        case 0x9d:
            STA(self, address_absolute_x(self));
            self->pc += 3;
            self->cycles += 5;
            break;
        case 0x99:
            STA(self, address_absolute_y(self));
            self->pc += 3;
            self->cycles += 5;
            break;
        case 0x81:
            STA(self, address_indexed_indirect(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        case 0x91:
            STA(self, address_indirect_indexed(self));
            self->pc += 2;
            self->cycles += 6;
            break;
        //STX
        case 0x86:
            STX(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x96:
            STX(self, address_zero_page_y(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x8e:
            STX(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        //STY
        case 0x84:
            STY(self, address_zero_page(self));
            self->pc += 2;
            self->cycles += 3;
            break;
        case 0x94:
            STY(self, address_zero_page_x(self));
            self->pc += 2;
            self->cycles += 4;
            break;
        case 0x8c:
            STY(self, address_absolute(self));
            self->pc += 3;
            self->cycles += 4;
            break;
        //TAX
        case 0xaa:
            TAX(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //TAY
        case 0xa8:
            TAY(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //TSX
        case 0xba:
            TSX(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //TXA
        case 0x8a:
            TXA(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //TSX
        case 0x9a:
            TXS(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        //TYA
        case 0x98:
            TYA(self);
            self->pc += 1;
            self->cycles += 2;
            break;
        default:
            printf("Invalid opcode %x at memory location %x!\n", self->opcode, self->pc);
            exit(EXIT_FAILURE);
	}
    self->iterations++;
}

/* Unit tests */
#ifdef UNITTEST

#include <assert.h>

void test_address_zero_page() {
    struct CPU self;
    init_cpu(&self);
    self.ram[1] = 0xff;
	self.ram[0xff] = 0xee;
    assert(*address_zero_page(&self) == 0xee);
}

void test_address_zero_page_x() {
    struct CPU self;
    init_cpu(&self);
    self.x = 0x01;
    self.ram[1] = 0xf0;
    self.ram[0xf1] = 0xee;
    assert(*address_zero_page_x(&self) == 0xee);
}

void test_address_zero_page_y() {
    struct CPU self;
    init_cpu(&self);
    self.y = 0x01;
    self.ram[1] = 0xf0;
    self.ram[0xf1] = 0xee;
    assert(*address_zero_page_y(&self) == 0xee);
}

void test_address_absolute() {
    struct CPU self;
    init_cpu(&self);
    self.ram[1] = 0x01;
    self.ram[2] = 0xf0;
    self.ram[0xf001] = 0xee;
    assert(*address_absolute(&self) == 0xee);
}

void test_address_absolute_x() {
    struct CPU self;
    init_cpu(&self);
    self.x = 0x01;
    self.ram[1] = 0x01;
    self.ram[2] = 0xf0;
    self.ram[0xf002] = 0xee;
    assert(*address_absolute_x(&self) == 0xee);
}

void test_address_absolute_y() {
    struct CPU self;
    init_cpu(&self);
    self.y = 0x01;
    self.ram[1] = 0x01;
    self.ram[2] = 0xf0;
    self.ram[0xf002] = 0xee;
    assert(*address_absolute_y(&self) == 0xee);
}

void test_address_indirect() {
    struct CPU self;
    init_cpu(&self);
    self.ram[0x01] = 0xf0;
    self.ram[0x02] = 0x00;
    self.ram[0xf000] = 0xfc;
    self.ram[0xf001] = 0xba;
    self.ram[0xbafc] = 0xee;
    assert(*address_indirect(&self) == 0xee);
}

void test_address_indexed_indirect() {
	struct CPU self;
	init_cpu(&self);
	self.ram[1] = 0x03;
	self.x = 0x01;
	self.ram[0x04] = 0x05;
	self.ram[0x05] = 0x07;
	self.ram[0x0705] = 0xee;
	assert(*address_indexed_indirect(&self) == 0xee);
}

void test_address_indirect_indexed() {
	struct CPU self;
	init_cpu(&self);
	self.ram[1] = 0x03;
	self.y = 0x01;
	self.ram[0x03] = 0x05;
	self.ram[0x04] = 0x07;
	self.ram[0x0706] = 0xee;
	assert(*address_indirect_indexed(&self) == 0xee);
}

void test_page_crossed() {
	assert(check_if_page_crossed(255,256) == 1);
	assert(check_if_page_crossed(200,300) == 1);
	assert(check_if_page_crossed(1, 1) == 0);
}

void test_SBC() {
    struct CPU self;
    //Test SBC w/ carry flag set
    init_cpu(&self);
    self.a = 5;
    self.ram[0] = 3;
    set_carry_flag(&self, 1);
    SBC(&self, &self.ram[0]);
	assert(self.a == 2);
    //Test SBC w/o carry flag set
    init_cpu(&self);
    self.a = 5;
    self.ram[0] = 3;
    SBC(&self, &self.ram[0]);
	assert(self.a == 1);
    //Test if SBC sets negative flag
    init_cpu(&self);
    self.a = 5;
    self.ram[0] = 6;
    set_carry_flag(&self, 1);
    SBC(&self, &self.ram[0]);
	assert(read_negative_flag(&self) == 1);
    //Test if SBC sets carry flag
    init_cpu(&self);
    self.a = 5;
    self.ram[0] = 6;
    set_carry_flag(&self, 1);
    SBC(&self, &self.ram[0]);
    assert(read_carry_flag(&self) == 1);
    //Test if SBC sets overflow flag
    init_cpu(&self);
    self.a = -128;
    self.ram[0] = 1;
    SBC(&self, &self.ram[0]);
    assert(read_overflow_flag(&self) == 1);
}

void test_pop_push() {
    struct CPU self;
    init_cpu(&self);
    uint8_t value = 1;
    push(&self, value);
    value = 2;
    push(&self, value);
    assert(pop(&self) == 2);
    assert(pop(&self) == 1);
}

void test_ROR() {
    struct CPU self;
    init_cpu(&self);
    //Test rotation
    set_carry_flag(&self, 1);
    self.ram[0xf] = 0b10000011;
    ROR(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b11000001);
    ROR(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b11100000);
    //Test carry flag, new bit seven should == current carry flag
    //and carry flag should get set to old bit zero.
    init_cpu(&self);
    self.ram[0xf] = 0b00000011;
    ROR(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b00000001);
    assert(read_carry_flag(&self) == 1);
}

void test_ROL() {
    struct CPU self;
    init_cpu(&self);
    //Test rotation
    set_carry_flag(&self, 1);
    self.ram[0xf] = 0b11000000;
    ROL(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b10000001);
    ROL(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b00000011);
    //Test carry flag, new bit zero == current carry flag
    //then carry flag == old bit seven.
    init_cpu(&self);
    self.ram[0xf] = 0b11000000;
    ROL(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b10000000);
    set_carry_flag(&self, 1);
    self.ram[0xf] = 0b01111111;
    ROL(&self, &self.ram[0xf]);
    assert(read_carry_flag(&self) == 0);
}

void test_LSR() {
    struct CPU self;
    init_cpu(&self);
    self.ram[0xf] = 0b00001111;
    LSR(&self, &self.ram[0xf]);
    assert(self.ram[0xf] == 0b00000111);
    assert(read_carry_flag(&self) == 1);
}

void run_cpu_tests() {
    test_address_zero_page();
    test_address_zero_page_x();
    test_address_zero_page_y();
    test_address_absolute();
    test_address_absolute_x();
    test_address_absolute_y();
    test_address_indirect();
	test_address_indexed_indirect();
    test_address_indirect_indexed();
	test_page_crossed();
	test_pop_push();
	test_SBC();
	test_ROR();
	test_ROL();
	test_LSR();
}
#endif

