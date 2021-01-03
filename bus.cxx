#include <cstdio>
#include <cassert>
#include "bus.hxx"
#include "rom.hxx"
#include "ppu.hxx"


void Bus::connect_rom(Rom *rom) {
    this->rom = rom;
}

void Bus::connect_ppu(Ppu *ppu) {
    this->ppu = ppu;
}

uint16_t Bus::trunacte_address(uint16_t address) {
    if(address <= RAM_ADDRESS_END)
        return address & RAM_ADDRESS_MAX_BITS;
    if(address > PPU_IO_ADDRESS_START & address <= PPU_IO_ADDRESS_END)
        return address & PPU_IO_ADDRESS_MAX_BITS;
    return address;
}

uint8_t Bus::read_ram(uint16_t address) {
    uint16_t trunacted_address = this->trunacte_address(address);
    if(trunacted_address <= RAM_ADDRESS_MAX_BITS)
        return this->ram.at(trunacted_address);
    if(trunacted_address >= ROM_IO_START)
        return this->rom->read_prgrom(trunacted_address);
}

//The NES CPU uses little endian binary representation, aka the least significant byte first.
uint16_t Bus::read_ram_16(uint16_t address) {
    return this->read_ram(address + 1) << 8 | this->read_ram(address);
}

void Bus::write_ram(uint16_t address, uint8_t value) {
    uint16_t trunacted_address = this->trunacte_address(address);
    if(trunacted_address <= RAM_ADDRESS_MAX_BITS)
        this->ram.at(address) = value;
}

void Bus::write_ram_16(uint16_t address, uint16_t value) {
    this->write_ram(address + 1, ((value & 0xff00) >> 8) );
    this->write_ram(address, value & 0xff);
}




void test_read_write() {
    auto rom = Rom("/home/notroot/nestest.nes");
    auto bus = Bus(&rom);
    bus.write_ram(10, 0xff);
    assert(bus.read_ram(10) == 0xff);
    bus.write_ram_16(10, 0xffee);
    assert(bus.read_ram(10) == 0xee);
    assert(bus.read_ram(11) == 0xff);
    assert(bus.read_ram_16(10) == 0xffee);
}

void test_bus() {
    test_read_write();
}