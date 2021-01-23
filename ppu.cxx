#include <iostream>
#include "config.hxx"
#include "ppu.hxx"
#include "bus.hxx"
#include "frame.hxx"
#include "color.hxx"

Tile::Tile(std::array<uint8_t, 16> data) {
    this->data = data;
}

bool Tile::get_pixel(int x, int y) {
    uint8_t tile_slice_a = this->data.at(y);
    uint8_t tile_slice_b = this->data.at(y + 8);
    uint8_t tile = tile_slice_a | tile_slice_b;
    return ((tile >> (7 - x)) & 0b1);
}

Ppu::Ppu() {
    this->controller = 0;
    this->mask = 0;
    this->status = 0;
    this->oam_address = 0;
    this->oam_data.fill(0);
    this->scroll = 0;
    this->address = 0;
}

void Ppu::connect_bus(Bus *bus) {
    this->bus = bus;
}

void Ppu::connect_frame(Frame *frame) {
    this->frame = frame;
}

void Ppu::reset() {
    this->set_status_flag(StatusFlag::vblank, true);
    this->set_status_flag(StatusFlag::sprite_overflow, true);
}

void Ppu::set_controller_flag(ControllerFlag flag, bool on) {
    if(on)
        this->controller |= static_cast<unsigned int>(flag);
    else
        this->controller &= ~static_cast<unsigned int>(flag);
}

void Ppu::write_controller(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to ppu controller value "
              << std::hex
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->controller = value;
}

bool Ppu::get_controller_flag(ControllerFlag flag) {
    return this->controller & static_cast<unsigned int>(flag);
}

uint8_t Ppu::read_controller() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading ppu controller"
              << std::endl;
    #endif
    return this->controller;
}

void Ppu::set_mask_flag(MaskFlag flag, bool on) {
    if(on)
        this->mask |= static_cast<unsigned int>(flag);
    else
        this->mask &= ~static_cast<unsigned int>(flag);
}

void Ppu::write_mask(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to ppu mask value "
              << std::hex
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->mask = value;
}

bool Ppu::get_mask_flag(MaskFlag flag) {
    return this->mask & static_cast<unsigned int>(flag);
}

uint8_t Ppu::read_mask() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading ppu mask" << std::endl;
    #endif
    return this->mask;
}

void Ppu::set_status_flag(StatusFlag flag, bool on) {
    if(on)
        this->status |= static_cast<unsigned int>(flag);
    else
        this->status &= ~static_cast<unsigned int>(flag);
}

void Ppu::write_status(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to ppu status value "
              << std::hex
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->status = value;
}

bool Ppu::get_status_flag(StatusFlag flag) {
    return this->status & static_cast<unsigned int>(flag);
}

uint8_t Ppu::read_status() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading ppu status" << std::endl;
    #endif
    return this->status;
}

void Ppu::write_oam_address(uint8_t value) {
    this->oam_address = value;
}

uint8_t Ppu::read_oam_address() {
    return this->oam_address;
}

void Ppu::write_oam_data(uint8_t value) {
    this->oam_data.at(this->oam_address) = value;
    this->oam_address++;
}

uint8_t Ppu::read_oam_data() {
    uint8_t value = this->oam_data.at(this->oam_address);
    if(!this->get_status_flag(StatusFlag::vblank))
        this->oam_address++;
    return value;
}

void Ppu::set_scroll_position(ScrollPosition position, uint8_t value) {
    if(position == ScrollPosition::horizontal)
        this->scroll = value | (this->scroll >> 8) << 8;
    else
        this->scroll = value << 8 | (this->scroll & 0xff);
}

void Ppu::write_scroll(uint8_t value) {
    if(!this->scroll_io_in_progress) {
        this->scroll_io_in_progress = true;
        this->scroll = value << 8;
    }
    else {
        this->scroll_io_in_progress = false;
        this->scroll |= value;
    }
}

uint8_t Ppu::get_scroll_position(ScrollPosition position) {
    if(position == ScrollPosition::horizontal)
        return this->scroll & 0xff;
    else
        return this->scroll >> 8;
}

uint16_t Ppu::read_scroll() {
    return this->scroll;
}

void Ppu::write_address(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to ppu address value "
              << std::hex
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    if(!this->address_io_in_progress) {
        this->address_io_in_progress = true;
        this->address = value << 8;
    }
    else {
        this->address_io_in_progress = false;
        this->address |= value;
    }
}

uint16_t Ppu::read_address() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading ppu address"
              << std::hex
              << static_cast<unsigned int>(this->address)
              << std::endl;
    #endif
    return this->address;
}

void Ppu::write_data(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to ppu data at address "
              << std::hex
              << static_cast<unsigned int>(this->address)
              << " value "
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->bus->write_vram(this->address, value);
    if(this->get_controller_flag(ControllerFlag::vram_increment)) {
        this->address += 32;
    }
    else {
        this->address++;
    }
}

uint8_t Ppu::read_data() {
    if(this->address <= NAMETABLE_START) {
        this->data = this->data_buffer;
        this->data_buffer = this->bus->read_vram(this->address);
    }
    else {
        this->data = this->bus->read_vram(this->address);
        this->data_buffer = this->bus->read_vram(this->address);
    }
    if(this->get_controller_flag(ControllerFlag::vram_increment)) {
        this->address += 32;
    }
    else {
        this->address++;
    }
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading ppu data at address "
              << std::hex
              << static_cast<unsigned int>(this->address)
              << " value "
              << static_cast<unsigned int>(this->data)
              << std::endl;
    #endif
    return this->data;
}

void Ppu::write_pallete_ram(uint16_t address, uint8_t value) {
    this->pallete_ram.at(address) = value;
}

uint8_t Ppu::read_pallete_ram(uint16_t address) {
    return this->pallete_ram.at(address);
}

bool Ppu::poll_nmi_interrupt() {
    if(this->get_status_flag(StatusFlag::vblank) & this->get_controller_flag(ControllerFlag::generate_nmi_on_vblank)) {
        return true;
    }
    else {
        return false;
    }
}

int Ppu::base_nametable_index() {
    int index = 0;
    if(this->get_controller_flag(ControllerFlag::base_nametable_address_1)){
        index += 1;
    }
    if(this->get_controller_flag(ControllerFlag::base_nametable_address_2)) {
        index += 2;
    }
    return index;
}

int Ppu::base_pattern_table_index() {
    int index = 0;
    if(this->get_controller_flag(ControllerFlag::sprite_pattern_table_address)) {
        index += 1;
    }
    return index;
}

Tile Ppu::get_tile(int pattern_table_index) {
    std::array<uint8_t, 16> tile_data;
    pattern_table_index *= 16;
    pattern_table_index += PATTERN_TABLE_SIZE * this->base_pattern_table_index();
    for(int i = pattern_table_index, e = 0; i < pattern_table_index + 16; i++, e++) {
        tile_data.at(e) = this->bus->read_vram(i);
    }
    return Tile(tile_data);
}

/* 262 scanlines per frame, scanline lasts for 341 cycles, scanline 241=vblank*/
void Ppu::tick(int cycles) {
    for(int cycle = 0; cycle < cycles * 3; cycle++) {
        if(this->scanline <= 240 & this->pixel <= 256) {
            int base_nametable_address = NAMETABLE_START + (NAMETABLE_SIZE * this->base_nametable_index());
            int nametable_x = this->pixel / 8;
            int nametable_y = (this->scanline / 8) * 32;
            int nametable_index = base_nametable_address + nametable_x + nametable_y;
            int pattern_table_index = this->bus->read_vram(nametable_index);
            auto tile = this->get_tile(pattern_table_index);
            int x = this->pixel % 7;
            int y = this->scanline % 7;
            if(tile.get_pixel(x, y)) {
                this->frame->set_pixel(this->pixel, this->scanline, Color::white);
            }
            else {
                this->frame->set_pixel(this->pixel, this->scanline, Color::black);
            }

        }
        if(this->pixel == 341) {
            this->pixel = 0;
            this->scanline++;
        }
        if(this->scanline >= 240) {
            this->set_status_flag(StatusFlag::vblank, true);
        }
        if(this->scanline == 262) {
            this->pixel = 0;
            this->scanline = 0;
            this->set_status_flag(StatusFlag::vblank, false);
        }
        this->pixel++;
    }
}