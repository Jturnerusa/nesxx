#include <iostream>
#include <cassert>
#include "bus.hxx"
#include "rom.hxx"
#include "ppu.hxx"

Bus::Bus() {
    this->ram.fill(0);
    this->vram.fill(0);
}

void Bus::connect_rom(Rom *rom) {
    this->rom = rom;
}

void Bus::connect_ppu(Ppu *ppu) {
    this->ppu = ppu;
}

uint16_t Bus::truncate_ram_address(uint16_t address) {
    if(address <= RAM_ADDRESS_END) {
        return address & RAM_ADDRESS_MAX_BITS;
    }
    if(address > PPU_IO_ADDRESS_START & address <= PPU_IO_ADDRESS_END) {
        return address & PPU_IO_ADDRESS_MAX_BITS;
    }
    return address;
}

uint8_t Bus::read_ram(uint16_t address) {
    address = this->truncate_ram_address(address);
    if(address <= RAM_ADDRESS_MAX_BITS) {
        return this->ram.at(address);
    }
    if(address >= PPU_IO_ADDRESS_START & address <= PPU_IO_ADDRESS_END) {
        switch (address) {
            case PPU_STATUS:
                return this->ppu->read_status();
            case PPU_OAM_DATA:
                return this->ppu->read_oam_data();
            case PPU_DATA:
                return this->ppu->read_data();
        }
    }
    if(address >= ROM_IO_START) {
        return this->rom->read_prgrom(address - ROM_IO_START);
    }
}

//The NES CPU uses little endian binary representation, aka the least significant byte first.
uint16_t Bus::read_ram_16(uint16_t address) {
    return this->read_ram(address + 1) << 8 | this->read_ram(address);
}

void Bus::write_ram(uint16_t address, uint8_t value) {
    address = this->truncate_ram_address(address);
    if(address <= RAM_ADDRESS_MAX_BITS) {
        this->ram.at(address) = value;
    }
    if(address >= PPU_IO_ADDRESS_START & address <= PPU_IO_ADDRESS_END) {
        switch (address) {
            case PPU_CONTROLLER:
                this->ppu->write_controller(value);
                break;
            case PPU_MASK:
                this->ppu->write_mask(value);
                break;
            case PPU_OAM_ADDRESS:
                this->ppu->write_oam_address(value);
                break;
            case PPU_OAM_DATA:
                this->ppu->write_oam_data(value);
                break;
            case PPU_SCROLL:
                this->ppu->write_scroll(value);
                break;
            case PPU_ADDRESS:
                this->ppu->write_address(value);
                break;
            case PPU_DATA:
                this->ppu->write_data(value);
                break;
        }
    }
}

void Bus::write_ram_16(uint16_t address, uint16_t value) {
    this->write_ram(address + 1, ( (value & 0xff00) >> 8) );
    this->write_ram(address, value & 0xff);
}

uint16_t Bus::truncate_vram_address(uint16_t address) {
    if(address >= NAMETABLE_START & address <= NAMETABLE_END) {
        return address & NAMETABLE_MAX_BITS;
    }
    if(address >= PALETTE_RAM_START) {
        return address & PALETTE_RAM_MAX_BITS;
    }
    return address;
}

uint16_t Bus::nametable_mirroring_calculator(uint16_t vram_address) {
    int vram_index = vram_address / 0x400;
    if(this->rom->mirroring_type == MirroringType::horizontal) {
        switch (vram_index) {
            case 0:
                return vram_address;
            case 1:
                return vram_address;
            case 2:
                return vram_address - 0x800;
            case 3:
                return vram_address - 0x800;
        }
    }
    if(rom->mirroring_type == MirroringType::vertical) {
        switch (vram_index) {
            case 0:
                return vram_address;
            case 1:
                return vram_address - 0x400;
            case 2:
                return vram_address - 0x400;
            case 3:
                return vram_address - 0x800;
        }
    }
    return vram_address;
}

uint8_t Bus::read_vram(uint16_t address) {
    address = this->truncate_vram_address(address);
    if(address <= PATTERN_TABLE_END) {
        return rom->read_chrrom(address);
    }
    if(address >= NAMETABLE_START & address <= NAMETABLE_MAX_BITS) {
        address -= NAMETABLE_START;
        address = this->nametable_mirroring_calculator(address);
        return this->vram.at(address);
    }
    if(address >= PALETTE_RAM_START) {
        return this->ppu->read_pallete_ram(address - PALETTE_RAM_START);
    }
}

void Bus::write_vram(uint16_t address, uint8_t value) {
    address = this->truncate_vram_address(address);
    if(address >= NAMETABLE_START & address <= NAMETABLE_MAX_BITS) {
        address -= NAMETABLE_START;
        address = this->nametable_mirroring_calculator(address);
        this->vram.at(address) = value;
    }
    if(address >= PALETTE_RAM_START) {
        this->ppu->write_pallete_ram(address - PALETTE_RAM_START, value);
    }
}

void Bus::vram_debug_view(int start, int stop) {
    for(int x = start; x < stop; x++) {
        std::cout << std::hex << static_cast<unsigned int>(read_vram(x)) << " ";
    }
    std::cout << std::endl;
}

uint16_t DummyBus::read_ram_16(uint16_t address) {
    
}


















