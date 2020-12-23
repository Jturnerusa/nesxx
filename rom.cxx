#include <fstream>
#include <cstdio>
#include "rom.hxx"

Rom::Rom(char* filepath){
    auto f = std::ifstream(filepath, std::ifstream::binary | std::ifstream::ate);
    if(!f.is_open())
        throw std::runtime_error("Error reading rom file");
    size_t length = f.tellg();
    f.seekg(f.beg);
    auto v = std::vector<char>(length);
    f.read(v.data(), length);
    std::string rom_header;
    rom_header.push_back(v.at(0));
    rom_header.push_back(v.at(1));
    rom_header.push_back(v.at(2));
    if(rom_header != "NES")
        throw std::runtime_error("Rom header is incorrect");
    for(auto x: v)
        this->rom.push_back(static_cast<uint8_t>(x));
    bool trainer_present = rom.at(5) & 0b100;
    int prgrom_size = rom.at(4) * PRGROM_UNIT_SIZE;
    int chrrom_size = rom.at(5) * CHRROM_UNIT_SIZE;
    int prgrom_offset = HEADER_SIZE + (trainer_present * TRAINER_SIZE);
    int chrrom_offset = prgrom_offset + prgrom_size;
    for(int x = 0; x < prgrom_size; x++)
        this->prgrom.push_back(rom.at(x + prgrom_offset));
    for(int x = 0; x < chrrom_size; x++)
        this->chrrom.push_back(rom.at(x + chrrom_offset));
    this->mapper = nrom;
}

uint16_t Rom::trunacate_prgrom_address(uint16_t address) {
    address -= 0x8000;
    return address & (PRGROM_UNIT_SIZE - 1);
}

uint8_t Rom::read_prgrom(uint16_t address) {
    uint16_t trunacated_ddress = this->trunacate_prgrom_address(address);
    return this->prgrom.at(trunacated_ddress);
}