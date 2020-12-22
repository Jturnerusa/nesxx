#include <cassert>
#include "cpu.hxx"

Cpu::Cpu(Bus *bus) {
    this->bus = bus;
    this->program_counter = 0;
    this->stack_pointer = 0xff;
    this->accumulator = 0;
    this->x = 0;
    this->y = 0;
    this->addressing_mode = AddressingMode::none;
    this->cycles = 0;
    this->iterations = 0;
}

void Cpu::set_processor_flag(ProcessorFlag flag, bool on) {
    if(on) {
        switch(flag) {
            case ProcessorFlag::carry:
                this->p |= 0b00000001;
                break;
            case ProcessorFlag::zero:
                this->p |= 0b00000010;
                break;
            case ProcessorFlag::interrupt:
                this->p |= 0b00000100;
                break;
            case ProcessorFlag::decimal:
                this->p |= 0b00001000;
                break;
            case ProcessorFlag::_break:
                this->p |= 0b00010000;
            case ProcessorFlag::overflow:
                this->p |= 0b01000000;
                break;
            case ProcessorFlag::negative:
                this->p |= 0b10000000;
                break;
        }
    }
    else {
        switch(flag) {
            case ProcessorFlag::carry:
                this->p &= 0b11111110;
                break;
            case ProcessorFlag::zero:
                this->p &= 0b11111101;
                break;
            case ProcessorFlag::interrupt:
                this->p &= 0b11111011;
                break;
            case ProcessorFlag::decimal:
                this->p &= 0b11110111;
                break;
            case ProcessorFlag::_break:
                this->p &= 0b11101111;
                break;
            case ProcessorFlag::overflow:
                this->p &= 0b10111111;
                break;
            case ProcessorFlag::negative:
                this->p &= 0b01111111;
                break;
        }
    }
}

bool Cpu::read_processor_flag(ProcessorFlag flag) {
    switch(flag) {
        case ProcessorFlag::carry:
            return this->p & 0b11111110;
        case ProcessorFlag::zero:
            return this->p & 0b11111101;
        case ProcessorFlag::interrupt:
            return this->p & 0b11111011;
        case ProcessorFlag::decimal:
            return this->p & 0b11110111;
        case ProcessorFlag::_break:
            return this->p & 0b11101111;
        case ProcessorFlag::overflow:
            return this->p & 0b10111111;
        case ProcessorFlag::negative:
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
        case AddressingMode::immediate:
            return this->address_immediate();
        case AddressingMode::zero_page:
            return this->address_zero_page();
        case AddressingMode::zero_page_x:
            return this->address_zero_page_x();
        case AddressingMode::zero_page_y:
            return this->address_zero_page_y();
        case AddressingMode::absolute:
            return this->address_absolute();
        case AddressingMode::absolute_x:
            return this->address_absolute_x();
        case AddressingMode::absolute_y:
            return this->address_absolute_y();
        case AddressingMode::indirect:
            return this->address_indirect();
        case AddressingMode::indirect_hardware_bug:
            return this->address_indirect_hardware_bug();
        case AddressingMode::indexed_indirect:
            return this->address_indexed_indirect();
        case AddressingMode::indirect_indexed:
            return this->address_indirect_indexed();
        case AddressingMode::none:
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

/* Reset interrupt */

void Cpu::reset() {
    this->x = 0;
    this->y = 0;
    this->accumulator = 0;
    this->stack_pointer = 0;
    this->program_counter = this->bus->read_ram_16(RESET_INTERRUPT_VECTOR);
}

/* Opcodes start here */

void Cpu::ADC() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    unsigned int sum = this->accumulator + value;
    this->set_processor_flag(ProcessorFlag::carry, sum > 0xff);
    this->set_processor_flag(ProcessorFlag::overflow, (this->accumulator ^ sum) ^ (this->accumulator ^ value) & 0x80);
    this->accumulator = static_cast<uint8_t>(sum);
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::AND() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator & value;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::ASL() {
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_seven = this->accumulator & 0x80;
        this->accumulator <<= 1;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
        this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
        this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_seven = value & 0x80;
        value <<= 1;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
        this->set_processor_flag(ProcessorFlag::zero, value == 0);
        this->set_processor_flag(ProcessorFlag::negative, value == 0);
        this->bus->write_ram(address, value);
    }
}

void Cpu::BIT() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    uint8_t result = this->accumulator & value;
    this->set_processor_flag(ProcessorFlag::zero, result == 0);
    this->set_processor_flag(ProcessorFlag::overflow, result & 0b01000000);
    this->set_processor_flag(ProcessorFlag::negative, result & 0x80);
}

void Cpu::BRK() {
    this->push_16(this->program_counter);
    this->push(this->p);
    this->program_counter = this->bus->read_ram_16(0xfffe);
    this->set_processor_flag(ProcessorFlag::_break, true);
}

void Cpu::CMP() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(ProcessorFlag::carry, this->accumulator >= value);
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == value);
    this->set_processor_flag(ProcessorFlag::negative, (this->accumulator - value) & 0x80);
}

void Cpu::CPX() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(ProcessorFlag::carry, this->x >= value);
    this->set_processor_flag(ProcessorFlag::zero, this->x == value);
    this->set_processor_flag(ProcessorFlag::negative, (this->x - value) & 0x80);
}

void Cpu::CPY() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->set_processor_flag(ProcessorFlag::carry, this->y >= value);
    this->set_processor_flag(ProcessorFlag::zero, this->y == value);
    this->set_processor_flag(ProcessorFlag::negative, (this->y - value) & 0x80);
}

void Cpu::DEC() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    value--;
    this->set_processor_flag(ProcessorFlag::zero, value == 0);
    this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
    this->bus->write_ram(address, value);
}

void Cpu::DEX() {
    this->x--;
    this->set_processor_flag(ProcessorFlag::zero, this->x == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->x & 0x80);
}

void Cpu::DEY() {
    this->y--;
    this->set_processor_flag(ProcessorFlag::zero, this->y == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->y & 0x80);
}

void Cpu::EOR(){
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator ^= value;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::INC(){
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    value++;
    this->set_processor_flag(ProcessorFlag::zero, value == 0);
    this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
    this->bus->write_ram(address, value);

}

void Cpu::INX(){
    this->x++;
    this->set_processor_flag(ProcessorFlag::zero, this->x == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->x & 0x80);
}

void Cpu::INY() {
    this->y++;
    this->set_processor_flag(ProcessorFlag::zero, this->y == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->y & 0x80);
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
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::LDX() {
    uint16_t address = this->resolve_address();
    this->x = this->bus->read_ram(address);
    this->set_processor_flag(ProcessorFlag::zero, this->x == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->x & 0x80);
}

void Cpu::LDY() {
    uint16_t address = this->resolve_address();
    this->y = this->bus->read_ram(address);
    this->set_processor_flag(ProcessorFlag::zero, this->y == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->y & 0x80);
}

void Cpu::LSR() {
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_zero = this->accumulator & 0b1;
        this->accumulator >>= 1;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        this->bus->write_ram(address, value);
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
    }
}

void Cpu::ORA() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator |= value;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
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
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_seven = this->accumulator & 0x80;
        this->accumulator <<= 1;
        if(this->read_processor_flag(ProcessorFlag::carry))
            this->accumulator &= 0b1;
        else
            this->accumulator &= 0b0;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_seven = value & 0x80;
        value <<= 1;
        if(this->read_processor_flag(ProcessorFlag::carry))
            value &= 0b1;
        else
            value &= 0b0;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
        this->bus->write_ram(address, value);
    }
}

void Cpu::ROR() {
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_zero = this->accumulator & 0b1;
        this->accumulator >>= 1;
        if(this->read_processor_flag(ProcessorFlag::carry))
            this->accumulator |= 0b1;
        else
            this->accumulator |= 0b0;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        if(this->read_processor_flag(ProcessorFlag::carry))
            value &= 0x10000000;
        else
            value &= 0x01111111;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
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
    if(this->addressing_mode == AddressingMode::accumulator) {
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
    this->set_processor_flag(ProcessorFlag::zero, this->x == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->x & 0x80);
}

void Cpu::TAY() {
    this->y = this->accumulator;
    this->set_processor_flag(ProcessorFlag::zero, this->y == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->y & 0x80);
}

void Cpu::TSX() {
    this->x = this->stack_pointer;
    this->set_processor_flag(ProcessorFlag::zero, this->x == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->x & 0x80);
}

void Cpu::TXA(){
    this->accumulator = this->x;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::TXS(){
    this->stack_pointer = this->x;
}

void Cpu::TYA(){
    this->accumulator = this->y;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::run_instruction() {
    this->addressing_mode = AddressingMode::none;
    this->opcode = this->bus->read_ram(this->program_counter);
    switch (this->opcode) {
        //ADC
        case 0x69:
            this->addressing_mode = AddressingMode::immediate;
            this->ADC();
            this->program_counter += 2;
            this->cycles += 2;
            break;
        case 0x65:
            this->addressing_mode = AddressingMode::zero_page;
            this->ADC();
            this->program_counter += 2;
            this->cycles += 3;
            break;
        case 0x75:
            this->addressing_mode = AddressingMode::zero_page_x;
            this->ADC();
            this->program_counter += 2;
            this->cycles += 4;
            break;
        case 0x6d:
            this->addressing_mode = AddressingMode::absolute;
            this->ADC();
            this->program_counter += 3;
            this->cycles += 4;
            break;
        case 0x7d:
            this->addressing_mode = AddressingMode::absolute_x;
            this->ADC();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0x79:
            this->addressing_mode = AddressingMode::absolute_y;
            this->ADC();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0x61:
            this->addressing_mode = AddressingMode::indexed_indirect;
            this->ADC();
            this->program_counter += 2;
            this->cycles += 6;
            break;
        case 0x71:
            this->addressing_mode = AddressingMode::indirect_indexed;
            this->ADC();
            this->program_counter += 2;
            this->cycles += 5;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        //AND
        case 0x29:
            this->addressing_mode = AddressingMode::immediate;
            this->AND();
            this->program_counter += 2;
            this->cycles += 2;
            break;
        case 0x25:
            this->addressing_mode = AddressingMode::zero_page;
            this->AND();
            this->program_counter += 2;
            this->cycles += 3;
            break;
        case 0x35:
            this->addressing_mode = AddressingMode::zero_page_x;
            this->AND();
            this->program_counter += 2;
            this->cycles += 4;
            break;
        case 0x2d:
            this->addressing_mode = AddressingMode::absolute;
            this->AND();
            this->program_counter += 3;
            this->cycles += 4;
            break;
        case 0x3d:
            this->addressing_mode = AddressingMode::absolute_x;
            this->AND();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0x39:
            this->addressing_mode = AddressingMode::absolute_y;
            this->AND();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0x21:
            this->addressing_mode = AddressingMode::indexed_indirect;
            this->AND();
            this->program_counter += 2;
            this->cycles += 6;
            break;
        case 0x31:
            this->addressing_mode = AddressingMode::indirect_indexed;
            this->AND();
            this->program_counter += 2;
            this->cycles += 5;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        //ASL
        case 0x0a:
            this->addressing_mode = AddressingMode::accumulator;
            this->ASL();
            this->program_counter += 1;
            this->cycles += 2;
            break;
        case 0x06:
            this->addressing_mode = AddressingMode::zero_page;
            this->ASL();
            this->program_counter += 2;
            this->cycles += 5;
            break;
        case 0x16:
            this->addressing_mode = AddressingMode::zero_page_x;
            this->ASL();
            this->program_counter += 2;
            this->cycles += 6;
            break;
        case 0x0e:
            this->addressing_mode = AddressingMode::absolute;
            this->ASL();
            this->program_counter += 3;
            this->cycles += 6;
            break;
        case 0x1e:
            this->addressing_mode = AddressingMode::absolute_x;
            this->ASL();
            this->program_counter += 3;
            this->cycles += 7;
            break;
        //BCC
        case 0x90:
            this->branch(!this->read_processor_flag(ProcessorFlag::carry));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BCS
        case 0xb0:
            this->branch(this->read_processor_flag(ProcessorFlag::carry));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BEQ
        case 0xf0:
            this->branch(this->read_processor_flag(ProcessorFlag::zero));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BIT
        case 0x24:
            this->addressing_mode = AddressingMode::zero_page;
            this->BIT();
            this->program_counter += 2;
            this->cycles += 3;
            break;
        case 0x2c:
            this->addressing_mode = AddressingMode::absolute;
            this->BIT();
            this->program_counter += 3;
            this->cycles += 4;
            break;
        //BMI
        case 0x30:
            this->branch(this->read_processor_flag(ProcessorFlag::negative));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BNE
        case 0xd0:
            this->branch(!this->read_processor_flag(ProcessorFlag::zero));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BPL
        case 0x10:
            this->branch(!this->read_processor_flag(ProcessorFlag::negative));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BRK
        case 0x00:
            this->BRK();
            this->cycles += 7;
            break;
        //BVC
        case 0x50:
            this->branch(!this->read_processor_flag(ProcessorFlag::overflow));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //BVS
        case 0x70:
            this->branch(this->read_processor_flag(ProcessorFlag::overflow));
            this->program_counter += 2;
            this->cycles += 2;
            if(this->page_crossed)
                this->cycles += 2;
            break;
        //CLC
        case 0x18:
            this->set_processor_flag(ProcessorFlag::carry, false);
            this->program_counter += 1;
            this->cycles += 2;
            break;
        //CLD
        case 0xd8:
            this->set_processor_flag(ProcessorFlag::decimal, false);
            this->program_counter += 1;
            this->cycles += 2;
            break;
        //CLI
        case 0x58:
            this->set_processor_flag(ProcessorFlag::interrupt, false);
            this->program_counter += 1;
            this->cycles += 2;
            break;
        //CLV
        case 0xb8:
            this->set_processor_flag(ProcessorFlag::overflow, false);
            this->program_counter += 1;
            this->cycles += 2;
            break;
        //CMP
        case 0xc9:
            this->addressing_mode = AddressingMode::immediate;
            this->CMP();
            this->program_counter += 2;
            this->cycles += 2;
            break;
        case 0xc5:
            this->addressing_mode = AddressingMode::zero_page;
            this->CMP();
            this->program_counter += 2;
            this->cycles += 3;
            break;
        case 0xd5:
            this->addressing_mode = AddressingMode::zero_page_x;
            this->CMP();
            this->program_counter += 2;
            this->cycles += 4;
            break;
        case 0xcd:
            this->addressing_mode = AddressingMode::absolute;
            this->CMP();
            this->program_counter += 3;
            this->cycles += 4;
            break;
        case 0xdd:
            this->addressing_mode = AddressingMode::absolute_x;
            this->CMP();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0xd9:
            this->addressing_mode = AddressingMode::absolute_y;
            this->CMP();
            this->program_counter += 3;
            this->cycles += 4;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        case 0xc1:
            this->addressing_mode = AddressingMode::indexed_indirect;
            this->CMP();
            this->program_counter += 2;
            this->cycles += 6;
            break;
        case 0xd1:
            this->addressing_mode = AddressingMode::indirect_indexed;
            this->CMP();
            this->program_counter += 2;
            this->cycles += 5;
            if(this->page_crossed)
                this->cycles += 1;
            break;
        //CPX
        case 0xe0:
            this->addressing_mode = AddressingMode::immediate;
            this->CPX();
            this->program_counter += 2;
            this->cycles += 2;
            break;
        case 0xe4:
            this->addressing_mode = AddressingMode::zero_page;
            this->CPX();
            this->program_counter += 2;
            this->cycles += 3;
            break;
        case 0xec:
            this->addressing_mode = AddressingMode::absolute;
            this->CPX();
            this->program_counter += 3;
            this->cycles += 4;
        //CPY
        //TYA
        case 0x98:
            this->TYA();
            this->program_counter += 1;
            this->cycles += 2;
            break;
    }
}