#ifndef CPU_H
#define CPU_H
#include <cstdint>
#include "config.hxx"

class Bus;

const uint16_t STACK_OFFSET = 0x100;
const uint16_t RESET_INTERRUPT_VECTOR = 0xfffc;
const uint16_t NMI_INTERRUPT_VECTOR = 0xfffa;

enum class ProcessorFlag {
    carry,
    zero,
    interrupt,
    decimal,
    _break,
    overflow,
    negative
};

enum class AddressingMode {
    accumulator,
    immediate,
    zero_page,
    zero_page_x,
    zero_page_y,
    absolute,
    absolute_x,
    absolute_y,
    indirect,
    indirect_hardware_bug,
    indexed_indirect,
    indirect_indexed,
    none
};

class Cpu {
#if UNITTEST==0
private:
#endif
    Bus *bus;
    uint16_t program_counter;
    uint8_t x, y, p, stack_pointer, accumulator, opcode;
    uint64_t cycles, iterations;
    AddressingMode addressing_mode;
    int opcode_cycles;
    bool is_processing_interrupt, page_crossed;
    void set_processor_flag(ProcessorFlag, bool);
    bool read_processor_flag(ProcessorFlag);
    uint16_t address_immediate();
    uint16_t address_zero_page();
    uint16_t address_zero_page_x();
    uint16_t address_zero_page_y();
    uint16_t address_absolute();
    uint16_t address_absolute_x();
    uint16_t address_absolute_y();
    uint16_t address_indirect();
    uint16_t address_indirect_hardware_bug();
    uint16_t address_indexed_indirect();
    uint16_t address_indirect_indexed();
    uint16_t resolve_address();
    int8_t relative_offset();
    void branch(bool);
    void push(uint8_t);
    void push_16(uint16_t);
    uint8_t pop();
    uint16_t pop_16();
    void nmi_interrupt();
    void ADC(bool = false);
    void AND();
    void ASL();
    void BIT();
    void BRK();
    void CMP();
    void CPX();
    void CPY();
    void DEC();
    void DEX();
    void DEY();
    void EOR();
    void INC();
    void INX();
    void INY();
    void JMP();
    void JSR();
    void LDA();
    void LDX();
    void LDY();
    void LSR();
    void ORA();
    void PHA();
    void PHP();
    void PLA();
    void PLP();
    void ROL();
    void ROR();
    void RTI();
    void RTS();
    void SBC();
    void STA();
    void STX();
    void STY();
    void TAX();
    void TAY();
    void TSX();
    void TXA();
    void TXS();
    void TYA();
#if UNITTEST==0
public:
#endif
    Cpu();
    void connect_bus(Bus *bus);
    void run_instruction();
    void reset();
    int get_opcode_cycles() {return this->opcode_cycles;};
};

void test_cpu();

#endif
