#ifndef CPU_H
#define CPU_H
#include <cstdint>
#include "config.hxx"

//Forward declaration
class Bus;

class Cpu {
private:
    static const uint16_t STACK_OFFSET = 0x100;
    static const uint16_t RESET_INTERRUPT_VECTOR = 0xfffc;
    static const uint16_t NMI_INTERRUPT_VECTOR = 0xfffa;
    enum class ProcessorFlag {
        carry     = 0b1,
        zero      = 0b10,
        interrupt = 0b100,
        decimal   = 0b1000,
        _break    = 0b10000,
        overflow  = 0b1000000,
        negative  = 0b10000000
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
    Bus *bus;
    uint16_t program_counter;
    uint8_t x, y, p, stack_pointer, accumulator, opcode;
    uint64_t cycles, iterations;
    AddressingMode addressing_mode;
    bool is_processing_interrupt, page_crossed;
    void set_processor_flag(ProcessorFlag, bool);
    bool check_if_page_crossed(uint16_t, uint16_t);
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
    bool branch(bool);
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
public:
    Cpu();
    void connect_bus(Bus *bus);
    void prepare_for_nestest();
    void run_for(int);
    void reset();
};

#ifdef UNITTEST
void run_cpu_tests();
#endif

#endif