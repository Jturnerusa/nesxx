#include <cassert>
#include "config.hxx"
#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"
#ifdef CPU_DEBUG_OUTPUT
#include <array>
#include "cpu_debug.hxx"
#endif

namespace cpu {

Cpu::Cpu() {
    this->program_counter = 0;
    this->accumulator = 0;
    this->x = 0;
    this->y = 0;
    this->p = 0;
    this->stack_pointer = 0;
    this->cycles = 0;
    this->iterations = 0;
    this->is_processing_interrupt = false;
    this->page_crossed = false;
    this->addressing_mode = AddressingMode::none;
}

void Cpu::connect_bus(bus::Bus *bus) {
    this->bus = bus;
}

void Cpu::prepare_for_nestest() {
    this->program_counter = 0xc000;
    this->stack_pointer = 0xfd;
    this->p = 0x24;
    this->cycles = 7;
}

bool Cpu::check_if_page_crossed(uint16_t a, uint16_t b) {
    if(((a >> 8) & 0xf) != ((b >> 8) & 0xf)) {
        return true;
    }
    else {
        return false;
    }
}

void Cpu::set_processor_flag(ProcessorFlag flag, bool on) {
    if(on) {
        this->p |= static_cast<unsigned int>(flag);
    }
    else {
        this->p &= ~static_cast<unsigned int>(flag);
    }
}

bool Cpu::read_processor_flag(ProcessorFlag flag) {
    return this->p & static_cast<unsigned int>(flag);
}

/* Addressing mode helper functions */

uint16_t Cpu::address_immediate() {
    return this->program_counter + 1;
}

uint16_t Cpu::address_zero_page() {
    return this->bus->read_ram(this->program_counter + 1);
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
    uint16_t address = this->bus->read_ram_16(this->program_counter + 1);
    if(this->check_if_page_crossed(address, address + this->x)) {
        this->page_crossed = true;
    }
    return address + this->x;
}

uint16_t Cpu::address_absolute_y() {
    uint16_t address = this->bus->read_ram_16(this->program_counter + 1);
    if(this->check_if_page_crossed(address, address + this->y)) {
        this->page_crossed = true;
    }
    return address + this->y;
}

uint16_t Cpu::address_indirect() {
    uint16_t address_absolute = this->bus->read_ram_16(this->program_counter + 1);
    return this->bus->read_ram_16(address_absolute);
}

uint16_t Cpu::address_indirect_hardware_bug() {
    /* An original 6502 does not correctly fetch the target address if the indirect vector falls on a page boundary
     * (e.g. $xxFF where xx is any value from $00 to $FF). In this case fetches the LSB from $xxFF as expected but
     * takes the MSB from $xx00. http://obelisk.me.uk/6502/reference.html#JMP */
    uint8_t absolute_higher = this->bus->read_ram(this->program_counter + 2);
    uint8_t absolute_lower = this->bus->read_ram(this->program_counter + 1);
    uint16_t absolute_a = absolute_higher << 8 | absolute_lower;
    //Wrap around the page boundary (page = 256 bytes) by overflowing the lower 8 bits of the address if it's 0xff.
    absolute_lower++;
    uint16_t absolute_b = absolute_higher << 8 | absolute_lower;
    uint16_t address_indirect = this->bus->read_ram(absolute_b) << 8 | this->bus->read_ram(absolute_a);
    return address_indirect;
}

uint16_t Cpu::address_indexed_indirect() {
    /* We cannot use read16 here because the zero page address needs to wrap around to the bottom of the page if
     * the address is 0xff. The parameter gets everything after the first 8 bits masked off since the read ram
     * function will interpret it as a 16bit value otherwise.*/
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    zero_page += this->x;
    uint16_t address = this->bus->read_ram((zero_page + 1) & 0xff) << 8;
    address |= this->bus->read_ram(zero_page);
    return address;
}

uint16_t Cpu::address_indirect_indexed() {
    uint8_t zero_page = this->bus->read_ram(this->program_counter + 1);
    uint16_t  address = this->bus->read_ram((zero_page + 1) & 0xff) << 8;
    address |= this->bus->read_ram(zero_page);
    if(this->check_if_page_crossed(address, address + this->y)) {
        this->page_crossed = true;
    }
    return address + this->y;
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
            assert("Tried to resolve address without an addressing mode" && 1 == 0);
    }
}

/* Helper functions for branch opcodes*/

int8_t Cpu::relative_offset() {
    return static_cast<int8_t>(this->bus->read_ram(this->program_counter + 1));
}

bool Cpu::branch(bool b) {
    if(b) {
        int8_t offset = this->relative_offset();
        //The + 2 is needed because the branch opcode will increase the pc by 2 regardless if the branch is taken.
        if(this->check_if_page_crossed(this->program_counter + 2, static_cast<uint16_t>(this->program_counter + offset))) {
            this->page_crossed = true;
        }
        this->program_counter += offset;
        return true;
    }
    else {
        return false;
    }
}

/* Stack functions */

void Cpu::push(uint8_t value) {
    this->bus->write_ram(this->stack_pointer + STACK_OFFSET, value);
    this->stack_pointer--;
}

void Cpu::push_16(uint16_t value) {
    this->push(value >> 8);
    this->push(value & 0xff);
}

uint8_t Cpu::pop() {
    this->stack_pointer++;
    uint8_t value = this->bus->read_ram(this->stack_pointer + STACK_OFFSET);
    return value;
}

uint16_t Cpu::pop_16() {
    uint16_t value = this->pop();
    value |= this->pop() << 8;
    return value;
}

/* Reset interrupt */

void Cpu::reset() {
    this->p = 0x34;
    this->accumulator, this->x, this->y = 0;
    this->stack_pointer = 0xfd;
    this->program_counter = this->bus->read_ram_16(RESET_INTERRUPT_VECTOR);
}

void Cpu::nmi_interrupt() {
    this->push_16(this->program_counter);
    this->push(this->p);
    this->set_processor_flag(ProcessorFlag::interrupt, true);
    this->program_counter = this->bus->read_ram_16(NMI_INTERRUPT_VECTOR);
}

/* Opcodes start here */

void Cpu::ADC(bool SBC) {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    if(SBC)
        value = ~value;
    unsigned int sum = this->accumulator + value + this->read_processor_flag(ProcessorFlag::carry);
    this->set_processor_flag(ProcessorFlag::carry, sum > 0xff);
    this->set_processor_flag(ProcessorFlag::overflow, (this->accumulator ^ sum) & (value ^ sum) & 0x80);
    this->accumulator = sum;
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::AND() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    this->accumulator &= value;
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
        this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
        this->bus->write_ram(address, value);
    }
}

void Cpu::BIT() {
    uint16_t address = this->resolve_address();
    uint8_t value = this->bus->read_ram(address);
    uint8_t result = value & this->accumulator;
    this->set_processor_flag(ProcessorFlag::zero, result == 0);
    this->set_processor_flag(ProcessorFlag::overflow, value & 0b01000000);
    this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
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
    this->program_counter = address;
}

void Cpu::JSR() {
    this->push_16(this->program_counter + 2);
    uint16_t address = this->resolve_address();
    this->program_counter = address;
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
        set_processor_flag(ProcessorFlag::carry, old_bit_zero);
        set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        this->bus->write_ram(address, value);
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
        this->set_processor_flag(ProcessorFlag::zero, value == 0);
    }
    this->set_processor_flag(ProcessorFlag::negative, false);

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
    this->push(this->p | 0b00010000);
}

void Cpu::PLA() {
    this->accumulator = this->pop();
    this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
    this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
}

void Cpu::PLP() {
    this->p = this->pop() & 0b11101111  | 0b00100000;
}

void Cpu::ROL(){
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_seven = this->accumulator & 0x80;
        this->accumulator <<= 1;
        this->accumulator |= this->read_processor_flag(ProcessorFlag::carry);
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
        this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
        this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_seven = value & 0x80;
        value <<= 1;
        value |= this->read_processor_flag(ProcessorFlag::carry);
        this->set_processor_flag(ProcessorFlag::carry, old_bit_seven);
        this->set_processor_flag(ProcessorFlag::zero, value == 0);
        this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
        this->bus->write_ram(address, value);
    }
}

void Cpu::ROR() {
    if(this->addressing_mode == AddressingMode::accumulator) {
        bool old_bit_zero = this->accumulator & 0b1;
        this->accumulator >>= 1;
        this->accumulator |= this->read_processor_flag(ProcessorFlag::carry) << 7;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
        this->set_processor_flag(ProcessorFlag::zero, this->accumulator == 0);
        this->set_processor_flag(ProcessorFlag::negative, this->accumulator & 0x80);
    }
    else {
        uint16_t address = this->resolve_address();
        uint8_t value = this->bus->read_ram(address);
        bool old_bit_zero = value & 0b1;
        value >>= 1;
        value |= this->read_processor_flag(ProcessorFlag::carry) << 7;
        this->set_processor_flag(ProcessorFlag::carry, old_bit_zero);
        this->set_processor_flag(ProcessorFlag::zero, value == 0);
        this->set_processor_flag(ProcessorFlag::negative, value & 0x80);
        this->bus->write_ram(address, value);
    }
}

void Cpu::RTI() {
    this->p = this->pop() & 0xef | 0x20;
    this->program_counter = this->pop_16();
}

void Cpu::RTS() {
    this->program_counter = this->pop_16();
}

void Cpu::SBC() {
    this->ADC(true);
}

void Cpu::STA() {
    uint16_t address = this->resolve_address();
    this->bus->write_ram(address, this->accumulator);
}

void Cpu::STX() {
    uint16_t address = this->resolve_address();
    this->bus->write_ram(address, this->x);
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

void Cpu::run_for(int cycles) {
    int cycle_counter = 0;
    while(cycle_counter < cycles) {
        this->addressing_mode = AddressingMode::none;
        this->page_crossed = false;
        this->opcode = this->bus->read_ram(this->program_counter);
        #ifdef CPU_DEBUG_OUTPUT
        int opcode_length = get_opcode_length(this->opcode);
        std::array<uint8_t, 3> args;
        for(int i = this->program_counter + 1, e = 0; i < this->program_counter + opcode_length; i++, e++) {
            args.at(e) = this->bus->read_ram(i);
        }
        print_debug_info(this->program_counter, this->opcode, opcode_length, args, this->accumulator, this->y, this->x, this->p,
                         this->stack_pointer, this->cycles, this->iterations);
        #endif
        #ifdef NESTEST
        debug_nestest_log_compare(this->program_counter, this->opcode, this->accumulator, this->y, this->x, this->p,
                                  this->stack_pointer, this->cycles, this->iterations);
        #endif
        int cycles_before_next_instruction = this->cycles;
        switch(this->opcode) {
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
                if(this->branch(!this->read_processor_flag(ProcessorFlag::carry))) {
                    this->cycles += 1;
                }
                this->program_counter += 2;
                this->cycles += 2;
                if(this->page_crossed)
                    this->cycles += 2;
                break;
                //BCS
            case 0xb0:
                if(this->branch(this->read_processor_flag(ProcessorFlag::carry))) {
                    this->cycles += 1;
                }
                this->program_counter += 2;
                this->cycles += 2;
                if(this->page_crossed)
                    this->cycles += 2;
                break;
                //BEQ
            case 0xf0:
                if(this->branch(this->read_processor_flag(ProcessorFlag::zero))) {
                    this->cycles += 1;
                }
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
                if(this->branch(this->read_processor_flag(ProcessorFlag::negative))) {
                    this->cycles += 1;
                }
                this->program_counter += 2;
                this->cycles += 2;
                if(this->page_crossed)
                    this->cycles += 2;
                break;
                //BNE
            case 0xd0:
                if(this->branch(!this->read_processor_flag(ProcessorFlag::zero))) {
                    this->cycles += 1;
                }
                this->program_counter += 2;
                this->cycles += 2;
                if(this->page_crossed)
                    this->cycles += 2;
                break;
                //BPL
            case 0x10:
                if(this->branch(!this->read_processor_flag(ProcessorFlag::negative))) {
                    this->cycles += 1;
                }
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
                if(this->branch(!this->read_processor_flag(ProcessorFlag::overflow))) {
                    this->cycles += 1;
                }
                this->program_counter += 2;
                this->cycles += 2;
                if(this->page_crossed)
                    this->cycles += 2;
                break;
                //BVS
            case 0x70:
                if(this->branch(this->read_processor_flag(ProcessorFlag::overflow))) {
                    this->cycles += 1;
                }
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
                break;
                //CPY
            case 0xc0:
                this->addressing_mode = AddressingMode::immediate;
                this->CPY();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0xc4:
                this->addressing_mode = AddressingMode::zero_page;
                this->CPY();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0xcc:
                this->addressing_mode = AddressingMode::absolute;
                this->CPY();
                this->program_counter += 3;
                this->cycles += 4;
                break;
                //DEC
            case 0xc6:
                this->addressing_mode = AddressingMode::zero_page;
                this->DEC();
                this->program_counter += 2;
                this->cycles += 5;
                break;
            case 0xd6:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->DEC();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0xce:
                this->addressing_mode = AddressingMode::absolute;
                this->DEC();
                this->program_counter += 3;
                this->cycles += 6;
                break;
            case 0xde:
                this->addressing_mode = AddressingMode::absolute_x;
                this->DEC();
                this->program_counter += 3;
                this->cycles += 7;
                break;
                //DEX
            case 0xca:
                this->DEX();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //DEY
            case 0x88:
                this->DEY();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //EOR
            case 0x49:
                this->addressing_mode = AddressingMode::immediate;
                this->EOR();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0x45:
                this->addressing_mode = AddressingMode::zero_page;
                this->EOR();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0x55:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->EOR();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0x4d:
                this->addressing_mode = AddressingMode::absolute;
                this->EOR();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0x5d:
                this->addressing_mode = AddressingMode::absolute_x;
                this->EOR();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0x59:
                this->addressing_mode = AddressingMode::absolute_y;
                this->EOR();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0x41:
                this->addressing_mode = AddressingMode::indexed_indirect;
                this->EOR();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x51:
                this->addressing_mode = AddressingMode::indirect_indexed;
                this->EOR();
                this->program_counter += 2;
                this->cycles += 5;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //INC
            case 0xe6:
                this->addressing_mode = AddressingMode::zero_page;
                this->INC();
                this->program_counter += 2;
                this->cycles += 5;
                break;
            case 0xf6:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->INC();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0xee:
                this->addressing_mode = AddressingMode::absolute;
                this->INC();
                this->program_counter += 3;
                this->cycles += 6;
                break;
            case 0xfe:
                this->addressing_mode = AddressingMode::absolute_x;
                this->INC();
                this->program_counter += 3;
                this->cycles += 7;
                break;
                //INX
            case 0xe8:
                this->INX();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //INY
            case 0xc8:
                this->INY();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //JMP
            case 0x4c:
                this->addressing_mode = AddressingMode::absolute;
                this->JMP();
                this->cycles += 3;
                break;
            case 0x6c:
                this->addressing_mode = AddressingMode::indirect_hardware_bug;
                this->JMP();
                this->cycles += 5;
                break;
                //JSR
            case 0x20:
                this->addressing_mode = AddressingMode::absolute;
                this->JSR();
                this->cycles += 6;
                break;
                //LDA
            case 0xa9:
                this->addressing_mode = AddressingMode::immediate;
                this->LDA();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0xa5:
                this->addressing_mode = AddressingMode::zero_page;
                this->LDA();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0xb5:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->LDA();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0xad:
                this->addressing_mode = AddressingMode::absolute;
                this->LDA();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0xbd:
                this->addressing_mode = AddressingMode::absolute_x;
                this->LDA();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0xb9:
                this->addressing_mode = AddressingMode::absolute_y;
                this->LDA();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0xa1:
                this->addressing_mode = AddressingMode::indexed_indirect;
                this->LDA();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0xb1:
                this->addressing_mode = AddressingMode::indirect_indexed;
                this->LDA();
                this->program_counter += 2;
                this->cycles += 5;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //LDX
            case 0xa2:
                this->addressing_mode = AddressingMode::immediate;
                this->LDX();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0xa6:
                this->addressing_mode = AddressingMode::zero_page;
                this->LDX();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0xb6:
                this->addressing_mode = AddressingMode::zero_page_y;
                this->LDX();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0xae:
                this->addressing_mode = AddressingMode::absolute;
                this->LDX();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0xbe:
                this->addressing_mode = AddressingMode::absolute_y;
                this->LDX();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //LDY
            case 0xa0:
                this->addressing_mode = AddressingMode::immediate;
                this->LDY();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0xa4:
                this->addressing_mode = AddressingMode::zero_page;
                this->LDY();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0xb4:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->LDY();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0xac:
                this->addressing_mode = AddressingMode::absolute;
                this->LDY();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0xbc:
                this->addressing_mode = AddressingMode::absolute_x;
                this->LDY();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //LSR
            case 0x4a:
                this->addressing_mode = AddressingMode::accumulator;
                this->LSR();
                this->program_counter += 1;
                this->cycles += 2;
                break;
            case 0x46:
                this->addressing_mode = AddressingMode::zero_page;
                this->LSR();
                this->program_counter += 2;
                this->cycles += 5;
                break;
            case 0x56:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->LSR();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x4e:
                this->addressing_mode = AddressingMode::absolute;
                this->LSR();
                this->program_counter += 3;
                this->cycles += 6;
                break;
            case 0x5e:
                this->addressing_mode = AddressingMode::absolute_x;
                this->LSR();
                this->program_counter += 3;
                this->cycles += 7;
                break;
                //NOP
            case 0xea:
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //ORA
            case 0x09:
                this->addressing_mode = AddressingMode::immediate;
                this->ORA();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0x05:
                this->addressing_mode = AddressingMode::zero_page;
                this->ORA();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0x15:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->ORA();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0x0d:
                this->addressing_mode = AddressingMode::absolute;
                this->ORA();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0x1d:
                this->addressing_mode = AddressingMode::absolute_x;
                this->ORA();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0x19:
                this->addressing_mode = AddressingMode::absolute_y;
                this->ORA();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0x01:
                this->addressing_mode = AddressingMode::indexed_indirect;
                this->ORA();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x11:
                this->addressing_mode = AddressingMode::indirect_indexed;
                this->ORA();
                this->program_counter += 2;
                this->cycles += 5;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //PHA
            case 0x48:
                this->PHA();
                this->program_counter += 1;
                this->cycles += 3;
                break;
                //PHP
            case 0x08:
                this->PHP();
                this->program_counter += 1;
                this->cycles += 3;
                break;
                //PLA
            case 0x68:
                this->PLA();
                this->program_counter += 1;
                this->cycles += 4;
                break;
                //PLP
            case 0x28:
                this->PLP();
                this->program_counter += 1;
                this->cycles += 4;
                break;
                //ROL
            case 0x2a:
                this->addressing_mode = AddressingMode::accumulator;
                this->ROL();
                this->program_counter += 1;
                this->cycles += 2;
                break;
            case 0x26:
                this->addressing_mode = AddressingMode::zero_page;
                this->ROL();
                this->program_counter += 2;
                this->cycles += 5;
                break;
            case 0x36:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->ROL();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x2e:
                this->addressing_mode = AddressingMode::absolute;
                this->ROL();
                this->program_counter += 3;
                this->cycles += 6;
                break;
            case 0x3e:
                this->addressing_mode = AddressingMode::absolute_x;
                this->ROL();
                this->program_counter += 3;
                this->cycles += 7;
                break;
                //ROR
            case 0x6a:
                this->addressing_mode = AddressingMode::accumulator;
                this->ROR();
                this->program_counter += 1;
                this->cycles += 2;
                break;
            case 0x66:
                this->addressing_mode = AddressingMode::zero_page;
                this->ROR();
                this->program_counter += 2;
                this->cycles += 5;
                break;
            case 0x76:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->ROR();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x6e:
                this->addressing_mode = AddressingMode::absolute;
                this->ROR();
                this->program_counter += 3;
                this->cycles += 6;
                break;
            case 0x7e:
                this->addressing_mode = AddressingMode::absolute_x;
                this->ROR();
                this->program_counter += 3;
                this->cycles += 7;
                break;
                //RTI
            case 0x40:
                this->RTI();
                this->cycles += 6;
                if(this->is_processing_interrupt) {
                    this->is_processing_interrupt = false;
                }
                break;
                //RTS
            case 0x60:
                this->RTS();
                this->program_counter += 1;
                this->cycles += 6;
                break;
                //SBC
            case 0xe9:
                this->addressing_mode = AddressingMode::immediate;
                this->SBC();
                this->program_counter += 2;
                this->cycles += 2;
                break;
            case 0xe5:
                this->addressing_mode = AddressingMode::zero_page;
                this->SBC();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0xf5:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->SBC();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0xed:
                this->addressing_mode = AddressingMode::absolute;
                this->SBC();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0xfd:
                this->addressing_mode = AddressingMode::absolute_x;
                this->SBC();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0xf9:
                this->addressing_mode = AddressingMode::absolute_y;
                this->SBC();
                this->program_counter += 3;
                this->cycles += 4;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
            case 0xe1:
                this->addressing_mode = AddressingMode::indexed_indirect;
                this->SBC();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0xf1:
                this->addressing_mode = AddressingMode::indirect_indexed;
                this->SBC();
                this->program_counter += 2;
                this->cycles += 5;
                if(this->page_crossed)
                    this->cycles += 1;
                break;
                //SEC
            case 0x38:
                this->set_processor_flag(ProcessorFlag::carry, true);
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //SED
            case 0xf8:
                this->set_processor_flag(ProcessorFlag::decimal, true);
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //SEI
            case 0x78:
                this->set_processor_flag(ProcessorFlag::interrupt, true);
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //STA
            case 0x85:
                this->addressing_mode = AddressingMode::zero_page;
                this->STA();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0x95:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->STA();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0x8d:
                this->addressing_mode = AddressingMode::absolute;
                this->STA();
                this->program_counter += 3;
                this->cycles += 4;
                break;
            case 0x9d:
                this->addressing_mode = AddressingMode::absolute_x;
                this->STA();
                this->program_counter += 3;
                this->cycles += 5;
                break;
            case 0x99:
                this->addressing_mode = AddressingMode::absolute_y;
                this->STA();
                this->program_counter += 3;
                this->cycles += 5;
                break;
            case 0x81:
                this->addressing_mode = AddressingMode::indexed_indirect;
                this->STA();
                this->program_counter += 2;
                this->cycles += 6;
                break;
            case 0x91:
                this->addressing_mode = AddressingMode::indirect_indexed;
                this->STA();
                this->program_counter += 2;
                this->cycles += 6;
                break;
                //STX
            case 0x86:
                this->addressing_mode = AddressingMode::zero_page;
                this->STX();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0x96:
                this->addressing_mode = AddressingMode::zero_page_y;
                this->STX();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0x8e:
                this->addressing_mode = AddressingMode::absolute;
                this->STX();
                this->program_counter += 3;
                this->cycles += 4;
                break;
                //STY
            case 0x84:
                this->addressing_mode = AddressingMode::zero_page;
                this->STY();
                this->program_counter += 2;
                this->cycles += 3;
                break;
            case 0x94:
                this->addressing_mode = AddressingMode::zero_page_x;
                this->STY();
                this->program_counter += 2;
                this->cycles += 4;
                break;
            case 0x8c:
                this->addressing_mode = AddressingMode::absolute;
                this->STY();
                this->program_counter += 3;
                this->cycles += 4;
                break;
                //TAX
            case 0xaa:
                this->TAX();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //TAY
            case 0xa8:
                this->TAY();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //TSX
            case 0xba:
                this->TSX();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //TXA
            case 0x8a:
                this->TXA();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //TXS
            case 0x9a:
                this->TXS();
                this->program_counter += 1;
                this->cycles += 2;
                break;
                //TYA
            case 0x98:
                this->TYA();
                this->program_counter += 1;
                this->cycles += 2;
                break;
            default:
                assert(("Invalid opcode", 1 == 0));
        }
        if(this->bus->ppu->poll_nmi_interrupt() & !this->is_processing_interrupt) {
            this->is_processing_interrupt = true;
            this->nmi_interrupt();
        }
        this->iterations++;
        cycle_counter += this->cycles - cycles_before_next_instruction;
    }
}

#ifdef UNITTEST

#endif

}