#include <iostream>
#include <fstream>
#include "rom.hxx"


void Rom::load_from_file(const char* filepath) {
    std::ifstream f(filepath, std::ifstream::binary | std::ifstream::ate);
    if(!f.is_open())
        throw std::runtime_error("Error reading rom file");
    size_t length = f.tellg();
    f.seekg(f.beg);
    std::vector<char> v(length);
    f.read(v.data(), length);
    std::string rom_header;
    rom_header.push_back(v.at(0));
    rom_header.push_back(v.at(1));
    rom_header.push_back(v.at(2));
    if(rom_header != "NES")
        throw std::runtime_error("Rom header is incorrect");
    std::cout << "Trying to load bytes from ROM!\n";
    for(auto x: v)
        this->rom.push_back(static_cast<uint8_t>(x));
    std::cout << "Initial ROM loading sucessful!\n";
    bool trainer_present = rom.at(6) & 0b100;
    int prgrom_size = rom.at(4) * PRGROM_UNIT_SIZE;
    int chrrom_size = rom.at(5) * CHRROM_UNIT_SIZE;
    int prgrom_offset = HEADER_SIZE + (trainer_present * TRAINER_SIZE);
    int chrrom_offset = prgrom_offset + prgrom_size;
    std::cout << "Preparing to read prgrom, total size " << prgrom_size << "\n";
    for(int x = 0; x < prgrom_size; x++)
        this->prgrom.push_back(rom.at(x + prgrom_offset));
    std::cout << "Loaded prgrom sucessfully!\n";
    std::cout << "Trying to read chrom, total size " << chrrom_size << "\n";
    for(int x = 0; x < chrrom_size; x++)
        this->chrrom.push_back(rom.at(x + chrrom_offset));
    std::cout << "Loaded chrrom successfully!\n";
    this->mapper = Mapper::nrom;
    std::cout << "Setting mapper type to NROM!\n";
    uint8_t flag = this->rom.at(6);
    if(flag & 0b1) {
        this->mirroring_type = MirroringType::vertical;
        std::cout << "Setting mirroring type to vertical!\n";
    }
    else {
        this->mirroring_type = MirroringType::horizontal;
        std::cout << "Setting mirroring type to horizontal!\n";
    }
}


uint16_t Rom::trunacate_prgrom_address(uint16_t address) {
    if(this->prgrom.size() == PRGROM_UNIT_SIZE) {
        return address & (PRGROM_UNIT_SIZE - 1);
    }
    return address;
}

uint8_t Rom::read_prgrom(uint16_t address) {
    address = this->trunacate_prgrom_address(address);
    return this->prgrom.at(address);
}

uint8_t Rom::read_chrrom(uint16_t address) {
    return this->chrrom.at(address);
}

#ifdef UNITTEST

DummyRom::DummyRom() {
    this->prgrom.resize(PRGROM_UNIT_SIZE * 2);
    this->chrrom.resize(CHRROM_UNIT_SIZE);
}

uint8_t DummyRom::read_prgrom(uint16_t address) {
    return this->prgrom.at(address);
}

uint8_t DummyRom::read_chrrom(uint16_t address) {
    return this->chrrom.at(address);
}

#endif