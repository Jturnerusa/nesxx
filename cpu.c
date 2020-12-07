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
	cpu->iterations = 0;
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
	if (cpu->p & 0b1) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_zero_flag(struct CPU *cpu) {
	if (cpu->p & 0b10) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_interrupt_flag(struct CPU *cpu) {
	if (cpu->p & 0b100) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_decimal_flag(struct CPU *cpu) {
	if (cpu->p & 0b1000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_break_flag(struct CPU *cpu) {
	if (cpu->p & 0b10000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_overflow_flag(struct CPU *cpu) {
	if (cpu->p & 0b01000000) {
		return 1;
	}
	else {
		return 0;
	}
}

int read_negative_flag(struct CPU *cpu) {
	if (cpu->p & 0b10000000) {
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

uint8_t *address_accumulator(struct CPU *cpu) {
	return &cpu->a;
}

uint8_t *address_immediate(struct CPU *cpu) {
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

uint8_t *address_relative(struct CPU *cpu) {
    return &cpu->ram[cpu->pc + 1];
}

uint8_t *address_absolute(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 2] << 8;
    address |= cpu->ram[cpu->pc + 1];
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_absolute_x(struct CPU *cpu) {
    uint16_t address = cpu->ram[cpu->pc + 2] << 8;
    address |= cpu->ram[cpu->pc + 1];
    address += cpu->x;
    cpu->page_crossed = check_if_page_crossed(cpu->pc, address);
    return &cpu->ram[address];
}

uint8_t *address_absolute_y(struct CPU *cpu) {
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

uint16_t address_location_absolute(struct CPU *cpu) {
	uint16_t address_absolute = cpu->ram[cpu->pc + 2] << 8;
	address_absolute |= cpu->ram[cpu->pc + 1];
	return address_absolute;
}

uint16_t address_location_indirect(struct CPU *cpu) {
	uint16_t absolute_address = cpu->ram[cpu->pc + 2] << 8;
	absolute_address |= cpu->ram[cpu->pc + 1];
	uint16_t indirect_location = cpu->ram[absolute_address + 1] << 8;
	indirect_location |= cpu->ram[absolute_address];
	return indirect_location;
}

/* Basic stack functions */

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

//Add with carry
void ADC(struct CPU *cpu, uint8_t *address) {
	unsigned int sum = cpu->a + *address + read_carry_flag(cpu);
	if (sum > 0xff) {
		set_carry_flag(cpu, 1);
	}
	if ((cpu->a ^ sum) & (*address ^ sum) & 0x80) {
		set_overflow_flag(cpu, 1);
	}
	cpu->a = sum;
	if (cpu->a == 0) {
		set_zero_flag(cpu, 1);
	}
	if (cpu->a >> 7) {
		set_negative_flag(cpu, 1);
	}
}

//Logical and accumulator & $address
void AND(struct CPU *cpu, uint8_t *address) {
	cpu->a &= *address;
	if (cpu->a == 0) {
		set_zero_flag(cpu, 1);
	}
	if (cpu->a & 0b10000000) {
		set_negative_flag(cpu, 1);
	}
}

//Arithmetic shift left
void ASL(struct CPU *cpu, uint8_t *address) {
    uint8_t old_bit_seven = *address >> 7;
    *address <<= 1;
    set_carry_flag(cpu, old_bit_seven);
    if (*address == 0) {
        set_zero_flag(cpu, 1);
    }
    if (*address >> 7) {
        set_negative_flag(cpu, 1);
    }
}

//Branch if carry flag is not set
void BCC(struct CPU *cpu, uint8_t *address) {
    if (read_carry_flag(cpu) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Branch if carry flag is set
void BCS(struct CPU *cpu, uint8_t *address) {
    if (read_carry_flag(cpu)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Branch if zero flag is set
void BEQ(struct CPU *cpu, uint8_t *address) {
    if (read_zero_flag(cpu)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Bit test
void BIT(struct CPU *cpu, uint8_t *address) {
    uint8_t result = *address & cpu->a;
    if (result == 0) {
        set_zero_flag(cpu, 1);
    }
    set_overflow_flag(cpu, *address & 0b01000000);
    set_negative_flag(cpu, *address & 0b10000000);
}

//Branch if negative flag is set
void BMI(struct CPU *cpu, uint8_t *address){
    if (read_negative_flag(cpu)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Branch if zero flag is not set
void BNE(struct CPU *cpu, uint8_t *address){
    if (read_zero_flag(cpu) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Branch if negative flag is not set
void BPL(struct CPU *cpu, uint8_t *address){
    if (read_negative_flag(cpu) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//TODO Force interrupt
//void BRK(struct CPU *cpu) {
//}

//Branch if overflow not set
void BVC(struct CPU *cpu, uint8_t *address) {
    if (read_overflow_flag(cpu) == 0) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Branch if overflow flag is set
void BVS(struct CPU *cpu, uint8_t *address) {
    if (read_overflow_flag(cpu)) {
        int8_t offset = (int8_t) *address;
        if (check_if_page_crossed(cpu->pc, cpu->pc + offset)) {
            cpu->page_crossed = 1;
        }
        cpu->pc += offset;
    }
}

//Clear carry flag
void CLC(struct CPU *cpu) {
    set_carry_flag(cpu, 0);
}

//Clear decimal flag
void CLD(struct CPU *cpu) {
    set_decimal_flag(cpu, 0);
}

//Clear interrupt flag
void CLI(struct CPU *cpu) {
    set_interrupt_flag(cpu, 0);
}

//Clear overflow flag
void CLV(struct CPU *cpu) {
    set_overflow_flag(cpu, 0);
}

//Compare accumulator $address
void CMP(struct CPU *cpu, uint8_t *address) {
    if (cpu->a >= *address) {
        set_carry_flag(cpu, 1);
    }
    if (cpu->a == *address) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Compare X $address
void CPX(struct CPU *cpu, uint8_t *address) {
    if (cpu->x >= *address) {
        set_carry_flag(cpu, 1);
    }
    if (cpu->x == *address) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->x & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Compare Y $address
void CPY(struct CPU *cpu, uint8_t *address) {
    if (cpu->y >= *address) {
        set_carry_flag(cpu, 1);
    }
    if (cpu->y == *address) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->y & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Decrement $address
void DEC(struct CPU *cpu, uint8_t *address) {
    *address--;
    if (*address == 0) {
        set_zero_flag(cpu, 1);
    }
    if (*address & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Decrement X
void DEX(struct CPU *cpu) {
    cpu->x--;
    if (cpu->x == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->x & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Decrement Y
void DEY(struct CPU *cpu) {
    cpu->y--;
    if (cpu->y == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->y & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

//Exclusive or accumulator and $address
void EOR(struct CPU *cpu, uint8_t *address)
{
    cpu->a ^= *address;
    if (cpu->a == 0) {
        set_zero_flag(cpu, 1);
    }
    if (cpu->a & 0b10000000) {
        set_negative_flag(cpu, 1);
    }
}

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
	*address >>= 1;
	set_carry_flag(cpu, old_bit_zero);
	if (*address == 0) {
		set_zero_flag(cpu, 1);
	}
	set_negative_flag(cpu, 1);
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
	*address = ~*address;
	ADC(cpu, address);
	*address = ~*address;
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

/* Debug output */

#if DEBUG==1
#include "cpu_debug.h"
#endif

/* Opcode lookup and run */

void run_instruction(struct CPU *cpu) {
    cpu->opcode = cpu->ram[cpu->pc];
    if (DEBUG) {
        print_debug_info(cpu);
    }
    switch(cpu->opcode) {
        //ADC
        case 0x69:
            ADC(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;      
        case 0x65:
            ADC(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0x75:
            ADC(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x6d:
            ADC(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0x7d:
            ADC(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x79:
            ADC(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x61:
            ADC(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x71:
            ADC(cpu, address_indirect_indexed(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //AND
        case 0x29:
            AND(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0x25:
            AND(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0x35:
            AND(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x2d:
            AND(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0x3d:
            AND(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x39:
            AND(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x21:
            AND(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
        case 0x31:
            AND(cpu, address_indirect_indexed(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //ASL 
        case 0x0a:
            ASL(cpu, address_accumulator(cpu));
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        case 0x06:
            ASL(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            break;
        case 0x16:
            ASL(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x0e:
            ASL(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 6;
            break;
        case 0x1e:
            ASL(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 7;
            break;
        //BCC
        case 0x90:
            BCC(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BCS
        case 0xb0:
            BCS(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BEQ
        case 0xf0:
            BEQ(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BIT
        case 0x24:
            BIT(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x2c:
            BIT(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        //BMI
        case 0x30:
            BMI(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BNE
        case 0xd0:
            BNE(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BPL
        case 0x10:
            BPL(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BVC
        case 0x50:
            BVC(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //BVS
        case 0x70:
            BVS(cpu, address_relative(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            if (cpu->page_crossed) {
                cpu->cycles += 2;
            }
            break;
        //CLC
        case 0x18:
            CLC(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //CLD
        case 0xd8:
            CLD(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //CLI
        case 0x58:
            CLI(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //CLV
        case 0xb8:
            CLV(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //CMP
        case 0xc9:
            CMP(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0xc5:
            CMP(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0xd5:
            CMP(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0xcd:
            CMP(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0xdd:
            CMP(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
        case 0xd9:
            CMP(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
    	case 0xc1:
			CMP(cpu, address_indexed_indirect(cpu));
			cpu->pc += 2;
			cpu->cycles += 6;
			break;
		case 0xd1:
			CMP(cpu, address_indirect_indexed(cpu));
			cpu->pc += 2;
			cpu->cycles += 5;
			if (cpu->page_crossed) {
				cpu->cycles += 1;
			}
			break;
		//CPX
		case 0xe0:
			CPX(cpu, address_immediate(cpu));
			cpu->pc += 2;
			cpu->cycles += 2;
			break;
		case 0xe4:
			CPX(cpu, address_zero_page(cpu));
			cpu->pc += 2;
			cpu->cycles += 3;
			break;
		case 0xec:
			CPX(cpu, address_absolute(cpu));
			cpu->pc += 3;
			cpu->cycles += 4;
			break;
		//CPY
		case 0xc0:
			CPY(cpu, address_immediate(cpu));
			cpu->pc += 2;
			cpu->cycles += 2;
			break;
		case 0xc4:
			CPY(cpu, address_zero_page(cpu));
			cpu->pc += 2;
			cpu->cycles += 3;
			break;
		case 0xcc:
			CPY(cpu, address_absolute(cpu));
			cpu->pc += 3;
			cpu->cycles += 4;
			break;
		//DEC
		case 0xc6:
			DEC(cpu, address_zero_page(cpu));
			cpu->pc += 2;
			cpu->cycles += 5;
			break;
		case 0xd6:
			DEC(cpu, address_zero_page_x(cpu));
			cpu->pc += 2;
			cpu->cycles += 6;
			break;
		case 0xce:
			DEC(cpu, address_absolute(cpu));
			cpu->pc += 3;
			cpu->cycles += 6;
			break;
		case 0xde:
			DEC(cpu, address_absolute_x(cpu));
			cpu->pc += 3;
			cpu->cycles += 7;
			break;
		//DEX
		case 0xca:
			DEX(cpu);
			cpu->pc += 1;
			cpu->cycles += 2;
			break;
		//DEY
		case 0x88:
			DEY(cpu);
			cpu->pc += 1;
			cpu->cycles += 2;
			break;
		//EOR
		case 0x49:
			EOR(cpu, address_immediate(cpu));
			cpu->pc += 2;
			cpu->cycles += 2;
			break;
		case 0x45:
			EOR(cpu, address_zero_page(cpu));
			cpu->pc += 2;
			cpu->cycles += 3;
			break;
		case 0x55:
			EOR(cpu, address_zero_page_x(cpu));
			cpu->pc += 2;
			cpu->cycles += 4;
			break;
		case 0x4d:
			EOR(cpu, address_absolute(cpu));
			cpu->pc += 3;
			cpu->cycles += 4;
			break;
		case 0x5d:
			EOR(cpu, address_absolute_x(cpu));
			cpu->pc += 3;
			cpu->cycles += 4;
			if (cpu->page_crossed) {
				cpu->cycles += 1;
			}
			break;
		case 0x59:
			EOR(cpu, address_absolute_y(cpu));
			cpu->pc += 3;
			cpu->cycles += 4;
			if (cpu->page_crossed) {
				cpu->cycles += 1;
			}
			break;
		case 0x41:
			EOR(cpu, address_indexed_indirect(cpu));
			cpu->pc += 2;
			cpu->cycles += 6;
			break;
		case 0x51:
			EOR(cpu, address_indirect_indexed(cpu));
			cpu->pc += 2;
			cpu->cycles += 5;
			if (cpu->page_crossed) {
				cpu->cycles += 1;
			}
			break;
		//INC
		case 0xe6:
			INC(cpu, address_zero_page(cpu));
			cpu->pc += 2;
			cpu->cycles += 5;
			break;
		case 0xf6:
			INC(cpu, address_zero_page_x(cpu));
			cpu->pc += 2;
			cpu->cycles += 6;
			break;
		case 0xee:
			INC(cpu, address_absolute(cpu));
			cpu->pc += 3;
			cpu->cycles += 6;
			break;
		case 0xfe:
			INC(cpu, address_absolute_x(cpu));
			cpu->pc += 3;
			cpu->cycles += 7;
			break;
		//INX
		case 0xe8:
			INX(cpu);
			cpu->pc += 1;
			cpu->cycles += 2;
			break;
		//INY
		case 0xc8:
			INY(cpu);
			cpu->pc += 1;
			cpu->cycles += 2;
			break;
		//JMP
		case 0x4c:
			JMP(cpu, address_location_absolute(cpu));
			cpu->cycles += 3;
			break;
		case 0x6c:
			JMP(cpu, address_location_indirect(cpu));
			cpu->cycles += 5;
			break;
		//JSR
		case 0x20:
			JSR(cpu, address_location_absolute(cpu));
			cpu->cycles += 6;
			break;
		//LDA
		case 0xa9:
		    LDA(cpu, address_immediate(cpu));
		    cpu->pc += 2;
		    cpu->cycles += 3;
		    break;
		case 0xa5:
		    LDA(cpu, address_zero_page(cpu));
		    cpu->pc += 2;
		    cpu->cycles += 3;
		    break;
        case 0xb5:
            LDA(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0xad:
            LDA(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0xbd:
            LDA(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0xb9:
            LDA(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0xa1:
            LDA(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0xb1:
            LDA(cpu, address_indirect_indexed(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //LDX
        case 0xa2:
            LDX(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0xa6:
            LDX(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0xb6:
            LDX(cpu, address_zero_page_y(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0xae:
            LDX(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0xbe:
            LDX(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //LDY
        case 0xa0:
            LDY(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0xa4:
            LDY(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0xb4:
            LDY(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0xac:
            LDY(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0xbc:
            LDY(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //LSR
        case 0x4a:
            LSR(cpu, address_accumulator(cpu));
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        case 0x46:
            LSR(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            break;
        case 0x56:
            LSR(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x4e:
            LSR(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 6;
            break;
        case 0x5e:
            LSR(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 7;
            break;
        //NOP
        case 0xea:
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //ORA
        case 0x09:
            ORA(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0x05:
            ORA(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0x15:
            ORA(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x0d:
            ORA(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0x1d:
            ORA(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x19:
            ORA(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0x01:
            ORA(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x11:
            ORA(cpu, address_indirect_indexed(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //PHA
        case 0x48:
            PHA(cpu);
            cpu->pc += 1;
            cpu->cycles += 3;
            break;
        //PHP
        case 0x08:
            PHP(cpu);
            cpu->pc += 1;
            cpu->cycles += 3;
            break;
        //PLA
        case 0x68:
            PLA(cpu);
            cpu->pc += 1;
            cpu->cycles += 4;
            break;
        //PLP
        case 0x28:
            PLP(cpu);
            cpu->pc += 1;
            cpu->cycles += 4;
            break;
        //ROL
        case 0x2a:
            ROL(cpu, address_accumulator(cpu));
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        case 0x26:
            ROL(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            break;
        case 0x36:
            ROL(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x2e:
            ROL(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 6;
            break;
        case 0x3e:
            ROL(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 7;
            break;
        //ROR
        case 0x6a:
            ROR(cpu, address_accumulator(cpu));
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        case 0x66:
            ROR(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 5;
            break;
        case 0x76:
            ROR(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x6e:
            ROR(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 6;
            break;
        case 0x7e:
            ROR(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 7;
            break;
        //RTI TODO
        case 0x40:
            cpu->pc += 1;
            cpu->cycles += 6;
            break;
        //RTS
        case 0x60:
            RTS(cpu);
            cpu->pc += 3;
            cpu->cycles += 6;
            break;
        //SBC
        case 0xe9:
            SBC(cpu, address_immediate(cpu));
            cpu->pc += 2;
            cpu->cycles += 2;
            break;
        case 0xe5:
            SBC(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0xf5:
            SBC(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0xed:
            SBC(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0xfd:
            SBC(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0xf9:
            SBC(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        case 0xe1:
            SBC(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0xf1:
            SBC(cpu, address_indirect_indexed(cpu));
            cpu->pc += 3;
            cpu->cycles += 5;
            if (cpu->page_crossed) {
                cpu->cycles += 1;
            }
            break;
        //SEC
        case 0x38:
            SEC(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //SED
        case 0xf8:
            SED(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //SEI
        case 0x78:
            SEI(cpu);
            cpu->pc += 1;
            cpu->cycles += 2;
            break;
        //STA
        case 0x85:
            STA(cpu, address_zero_page(cpu));
            cpu->pc += 2;
            cpu->cycles += 3;
            break;
        case 0x95:
            STA(cpu, address_zero_page_x(cpu));
            cpu->pc += 2;
            cpu->cycles += 4;
            break;
        case 0x8d:
            STA(cpu, address_absolute(cpu));
            cpu->pc += 3;
            cpu->cycles += 4;
            break;
        case 0x9d:
            STA(cpu, address_absolute_x(cpu));
            cpu->pc += 3;
            cpu->cycles += 5;
            break;
        case 0x99:
            STA(cpu, address_absolute_y(cpu));
            cpu->pc += 3;
            cpu->cycles += 5;
            break;
        case 0x81:
            STA(cpu, address_indexed_indirect(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
        case 0x91:
            STA(cpu, address_indirect_indexed(cpu));
            cpu->pc += 2;
            cpu->cycles += 6;
            break;
	}
	cpu->iterations++;
}

/* Unit tests */
#ifdef UNITTEST

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

void test_address_absolute() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf001] = 0xee;
    assert(*address_absolute(&cpu) == 0xee);
}

void test_address_absolute_x() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.x = 0x01;
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf002] = 0xee;
    assert(*address_absolute_x(&cpu) == 0xee);
}

void test_address_absolute_y() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.y = 0x01;
    cpu.ram[1] = 0x01;
    cpu.ram[2] = 0xf0;
    cpu.ram[0xf002] = 0xee;
    assert(*address_absolute_y(&cpu) == 0xee);
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
    cpu.ram[0] = 6;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
	assert(read_negative_flag(&cpu) == 1);
    //Test if SBC sets carry flag
    init_cpu(&cpu);
    cpu.a = 5;
    cpu.ram[0] = 6;
    set_carry_flag(&cpu, 1);
    SBC(&cpu, &cpu.ram[0]);
    assert(read_carry_flag(&cpu) == 1);
    //Test if SBC sets overflow flag
    init_cpu(&cpu);
    cpu.a = -128;
    cpu.ram[0] = 1;
    SBC(&cpu, &cpu.ram[0]);
    assert(read_overflow_flag(&cpu) == 1);
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

void test_LSR() {
    struct CPU cpu;
    init_cpu(&cpu);
    cpu.ram[0xf] = 0b00001111;
    LSR(&cpu, &cpu.ram[0xf]);
    assert(cpu.ram[0xf] == 0b00000111);
    assert(read_carry_flag(&cpu) == 1);
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

