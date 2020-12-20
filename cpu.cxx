#include <iostream>
#include <cassert>
#include "cpu.hxx"

Cpu::Cpu(Bus *bus) {
    this->bus = bus;
    this->program_counter = 0;
    this->stack_pointer = 0xff;
    this->accumulator = 0;
    this->x = 0;
    this->y = 0;
    this->cycles = 0;
    this->iterations = 0;
}

void Cpu::set_processor_flag(Processor_flag flag, bool on) {
    if(on) {
        switch(flag) {
            case carry:
                this->p |= 0b00000001;
                break;
            case zero:
                this->p |= 0b00000010;
                break;
            case interrupt:
                this->p |= 0b00000100;
                break;
            case decimal:
                this->p |= 0b00001000;
                break;
            case _break:
                this->p |= 0b00010000;
            case overflow:
                this->p |= 0b01000000;
                break;
            case negative:
                this->p |= 0b10000000;
                break;
        }
    }
    else {
        switch(flag) {
            case carry:
                this->p &= 0b11111110;
                break;
            case zero:
                this->p &= 0b11111101;
                break;
            case interrupt:
                this->p &= 0b11111011;
                break;
            case decimal:
                this->p &= 0b11110111;
                break;
            case _break:
                this->p &= 0b11101111;
                break;
            case overflow:
                this->p &= 0b10111111;
                break;
            case negative:
                this->p &= 0b01111111;
                break;
        }
    }
}

bool Cpu::read_processor_flag(Processor_flag flag) {
    switch(flag) {
        case carry:
            return this->p & 0b11111110;
        case zero:
            return this->p & 0b11111101;
        case interrupt:
            return this->p & 0b11111011;
        case decimal:
            return this->p & 0b11110111;
        case _break:
            return this->p & 0b11101111;
        case overflow:
            return this->p & 0b10111111;
        case negative:
            return this->p & 0b01111111;
    }
}

/* Addressing mode helper functions */

uint16_t Cpu::address_immediate() {
    return this->program_counter + 1;
}

uint16_t Cpu::address_zero_page() {
    return static_cast<uint16_t>(this->bus->read_ram(this->program_counter + 1));
}

uint16_t Cpu::address_zero_page_x() {
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    zero_page += this->x;
    return static_cast<uint16_t>(zero_page);
}

uint16_t Cpu::address_zero_page_y() {
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    zero_page += this->y;
    return static_cast<uint16_t>(zero_page);
}

uint16_t Cpu::address_absolute() {
    return this->bus->read_ram_16(this->program_counter + 1);
}

uint16_t Cpu::address_absolute_x() {
    return this->bus->read_ram_16(this->program_counter + 1) + this->x;
}

uint16_t Cpu::address_absolute_y() {
    return this->bus->read_ram_16(this->program_counter + 1) + this->y;
}

uint16_t Cpu::address_indirect() {
    uint16_t address_absolute = this->bus->read_ram_16(this->program_counter + 1);
    return this->bus->read_ram_16(address_absolute);
}

uint16_t Cpu::address_indirect_hardware_bug() {
    /* An original 6502 does not correctly fetch the target address if the indirect vector falls on a page boundary 
    (e.g. $xxFF where xx is any value from $00 to $FF). In this case fetches the LSB from $xxFF as expected but takes 
    the MSB from $xx00. http://obelisk.me.uk/6502/reference.html#JMP */
    uint8_t absolute_higher = this->bus->read_ram(this->program_counter + 2);
    uint8_t absolute_lower = this->bus->read_ram(this->program_counter + 1);
    uint16_t absolute_a = absolute_higher << 8 | absolute_lower;
    //Wrap around the page boundary (page = 256 bytes) by overflowing the lower 8 bits of the address if it's 0xff.
    absolute_lower++;
    uint16_t absolute_b = absolute_higher << 8 | absolute_lower;
    uint16_t address_indirect = this->bus->read_ram(absolute_a) << 8 | this->bus->read_ram(absolute_b);
    return address_indirect;  
}

uint16_t Cpu::address_indexed_indirect() {
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    zero_page += this->x;
    return this->bus->read_ram_16(zero_page);
}

uint16_t Cpu::address_indirect_indexed() {
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    return this->bus->read_ram_16(zero_page) + this->y;
}

uint16_t Cpu::resolve_address() {
    switch(this->addressing_mode) {
        case immediate:
            return this->address_immediate();
        case zero_page:
            return this->address_zero_page();
        case zero_page_x:
            return this->address_zero_page_x();
        case zero_page_y:
            return this->address_zero_page_y();
        case absolute:
            return this->address_absolute();
        case absolute_x:
            return this->address_absolute_x();
        case absolute_y:
            return this->address_absolute_y();
        case indirect:
            return this->address_indirect();
        case indirect_hardware_bug:
            return this->address_indirect_hardware_bug();
        case indexed_indirect:
            return this->address_indexed_indirect();
        case indirect_indexed:
            return this->address_indirect_indexed();
        case none:
            assert(1 == 0);
    }
}

/* Helper functions for branch opcodes*/

int8_t Cpu::relative_offset() {
    return static_cast<int8_t>(this->bus->read_ram(this->program_counter + 1));
}

void Cpu::branch(bool b) {
    if(b) {
        int8_t offset = this->relative_offset();
        this->program_counter += offset;
    }
}

/* Stack functions */

void Cpu::push(uint8_t value) {
    this->bus->write_ram(this->stack_pointer, value);
    this->stack_pointer--;
}

void Cpu::push_16(uint16_t value) {
    this->bus->write_ram_16(this->stack_pointer, value);
    this->stack_pointer -= 2;
}

uint8_t Cpu::pop() {
    uint8_t value = this->bus->read_ram(this->stack_pointer);
    this->stack_pointer--;
    return value;
}

uint16_t Cpu::pop_16() {
    uint16_t value = this->bus->read_ram_16(this->stack_pointer);
    this->stack_pointer -= 2;
    return value;
}

/* Opcodes start here */

void Cpu::ADC() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    unsigned int sum = this->accumulator + value;
    this->set_processor_flag(carry, sum > 0xff);
    this->set_processor_flag(overflow, (this->accumulator ^ sum) ^ (this->accumulator ^ value) & 0x80);
    this->accumulator = static_cast<uint8_t>(sum);
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::AND() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator & value;
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::ASL() {
    if(this->addressing_mode == accumulator) {
        bool old_bit_seven = this->accumulator & 0x80;
        this->accumulator <<= 1;
        this->set_processor_flag(carry, old_bit_seven);
        this->set_processor_flag(zero, this->accumulator == 0);
        this->set_processor_flag(negative, this->accumulator & 0x80);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_seven = value & 0x80;
        value <<= 1;
        this->set_processor_flag(carry, old_bit_seven);
        this->set_processor_flag(zero, value == 0);
        this->set_processor_flag(negative, value == 0);
        this->bus->write_ram(address, value);
    }
}

void Cpu::BIT() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    uint8_t result = this->accumulator & value;
    this->set_processor_flag(zero, result == 0);
    this->set_processor_flag(overflow, result & 0b01000000);
    this->set_processor_flag(negative, result & 0x80);
}

void Cpu::BRK() {
    this->push_16(this->program_counter);
    this->push(this->p);
    this->program_counter = this->bus->read_ram_16(0xfffe);
    this->set_processor_flag(_break, true);
}

void Cpu::CMP() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(carry, this->accumulator >= value);
    this->set_processor_flag(zero, this->accumulator == value);
    this->set_processor_flag(negative, (this->accumulator - value) & 0x80);
}

void Cpu::CPX() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(carry, this->x >= value);
    this->set_processor_flag(zero, this->x == value);
    this->set_processor_flag(negative, (this->x - value) & 0x80);
}

void Cpu::CPY() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(carry, this->y >= value);
    this->set_processor_flag(zero, this->y == value);
    this->set_processor_flag(negative, (this->y - value) & 0x80);
}

void Cpu::DEC() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    value--;
    this->set_processor_flag(zero, value == 0);
    this->set_processor_flag(negative, value & 0x80);
    this->bus->write_ram(address, value);
}

void Cpu::DEX() {
    this->x--;
    this->set_processor_flag(zero, this->x == 0);
    this->set_processor_flag(negative, this->x & 0x80);
}

void Cpu::DEY() {
    this->y--;
    this->set_processor_flag(zero, this->y == 0);
    this->set_processor_flag(negative, this->y & 0x80);
}

void Cpu::EOR(){
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator ^= value;
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::INC(){
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    value++;
    this->set_processor_flag(zero, value == 0);
    this->set_processor_flag(negative, value & 0x80);
    this->bus->write_ram(address, value);

}

void Cpu::INX(){
    this->x++;
    this->set_processor_flag(zero, this->x == 0);
    this->set_processor_flag(negative, this->x & 0x80);
}

void Cpu::INY() {
    this->y++;
    this->set_processor_flag(zero, this->y == 0);
    this->set_processor_flag(negative, this->y & 0x80);
}

void Cpu::JMP() {
    uint16_t address = this->resolve_address();
    this->program_counter = this->bus->read_ram_16(address);
}

void Cpu::JSR() {
    this->push_16(this->program_counter + 1);
    uint16_t address = this->resolve_address();
    this->program_counter = this->bus->read_ram_16(address);
}

void Cpu::LDA() {
    uint16_t address = this->resolve_address();
    this->accumulator = this->bus->read_ram(address);
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::LDX() {
    uint16_t address = this->resolve_address();
    this->x = this->bus->read_ram(address);
    this->set_processor_flag(zero, this->x == 0);
    this->set_processor_flag(negative, this->x & 0x80);
}

void Cpu::LDY() {
    uint16_t address = this->resolve_address();
    this->y = this->bus->read_ram(address);
    this->set_processor_flag(zero, this->y == 0);
    this->set_processor_flag(negative, this->y & 0x80);
}

void Cpu::LSR() {
    if(this->addressing_mode == accumulator) {
        bool old_bit_zero = this->accumulator & 0b1;
        this->accumulator >>= 1;
        this->set_processor_flag(carry, old_bit_zero);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        this->bus->write_ram(address, value);
        this->set_processor_flag(carry, old_bit_zero);
    }
}

void Cpu::ORA() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator |= value;
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::PHA() {
    this->push(this->accumulator);
}

void Cpu::PHP() {
    this->push(this->p);
}

void Cpu::PLA() {
    this->accumulator = this->pop();
}

void Cpu::PLP() {
    this->p = this->pop() & 0b11101111  | 0b00100000;
}

void Cpu::ROL(){
    if(this->addressing_mode == accumulator) {
        bool old_bit_seven = this->accumulator & 0x80;
        this->accumulator <<= 1;
        if(this->read_processor_flag(carry))
            this->accumulator &= 0b1;
        else
            this->accumulator &= 0b0;
        this->set_processor_flag(carry, old_bit_seven);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_seven = value & 0x80;
        value <<= 1;
        if(this->read_processor_flag(carry))
            value &= 0b1;
        else
            value &= 0b0;
        this->set_processor_flag(carry, old_bit_seven);
        this->bus->write_ram(address, value);
    }
}

void Cpu::ROR() {
    if(this->addressing_mode == accumulator) {
        bool old_bit_zero = this->accumulator & 0b1;
        this->accumulator >>= 1;
        if(this->read_processor_flag(carry))
            this->accumulator |= 0b1;
        else
            this->accumulator |= 0b0;
        this->set_processor_flag(carry, old_bit_zero);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        if(this->read_processor_flag(carry))
            value &= 0x10000000;
        else
            value &= 0x01111111;
        this->set_processor_flag(carry, old_bit_zero);
    }
}

void Cpu::RTI() {
    this->p = this->pop();
    this->program_counter = this->pop_16();
}

void Cpu::RTS() {
    this->program_counter = this->pop_16() + 1;
}

void Cpu::SBC() {
    if(this->addressing_mode == accumulator) {
        this->accumulator = ~this->accumulator;
        //this->ADC();
        this->accumulator = ~this->accumulator;
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        this->bus->write_ram(address, ~value);
        //this->ADC();
        value = this->bus->read_ram(address);
        this->bus->write_ram(address, ~value);
    }
}

void Cpu::SEC() {
    this->set_processor_flag(carry, true);
}

void Cpu::SED() {
    this->set_processor_flag(decimal, true);
}

void Cpu::SEI() {
    this->set_processor_flag(interrupt, true);
}

void Cpu::STA() {
    uint16_t address = this->resolve_address();
    this->bus->write_ram(address, this->accumulator);
}

void Cpu::STY() {
    uint16_t address = this->resolve_address();
    this->bus->write_ram(address, this->y);
}

void Cpu::TAX() {
    this->x = this->accumulator;
    this->set_processor_flag(zero, this->x == 0);
    this->set_processor_flag(negative, this->x & 0x80);
}

void Cpu::TAY() {
    this->y = this->accumulator;
    this->set_processor_flag(zero, this->y == 0);
    this->set_processor_flag(negative, this->y & 0x80);
}

void Cpu::TSX() {
    this->x = this->stack_pointer;
    this->set_processor_flag(zero, this->x == 0);
    this->set_processor_flag(negative, this->x & 0x80);
}

void Cpu::TXA(){
    this->accumulator = this->x;
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}

void Cpu::TXS(){
    this->stack_pointer = this->x;
}

void Cpu::TYA(){
    this->accumulator = this->y;
    this->set_processor_flag(zero, this->accumulator == 0);
    this->set_processor_flag(negative, this->accumulator & 0x80);
}
