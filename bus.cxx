#include "bus.hxx"

Bus::Bus(){
}

uint8_t Bus::read_ram(uint16_t address) {
    return this->ram.at(address);
}

//The NES CPU uses little endian binary representation, aka the least signifigant byte first. 
uint16_t Bus::read_ram_16(uint16_t address) {
    return this->ram.at(address + 1) << 8 | this->ram.at(address);
}

void Bus::write_ram(uint16_t address, uint8_t value) {
    this->ram.at(address) = value;
}

void Bus::write_ram_16(uint16_t address, uint16_t value) {
    this->ram.at(address + 1) = (value & 0xff00) >> 8;
    this->ram.at(address) = value & 0xff;
}
