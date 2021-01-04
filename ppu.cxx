#include "ppu.hxx"
#include "bus.hxx"

void Ppu::connect_bus(Bus *bus) {
    this->bus = bus;
}

void Ppu::set_controller_flag(ControllerFlag flag, bool on) {
    if(on)
        this->controller |= (0b1 << static_cast<int>(flag));
    else
        this->controller &= ~(0b1 << static_cast<int>(flag));
}

void Ppu::write_controller(uint8_t value) {
    this->controller = value;
}

bool Ppu::get_controller_flag(ControllerFlag flag) {
    return this->controller & (0b1 << static_cast<int>(flag));
}

uint8_t Ppu::read_controller() {
    return this->controller;
}

void Ppu::set_mask_flag(MaskFlag flag, bool on) {
    if(on)
        this->mask |= (0b1 << static_cast<int>(flag));
    else
        this->mask &= ~(0b1 << static_cast<int>(flag));
}

void Ppu::write_mask(uint8_t value) {
    this->mask = value;
}

bool Ppu::get_mask_flag(MaskFlag flag) {
    return this->mask & (0b1 << static_cast<int>(flag));
}

uint8_t Ppu::read_mask() {
    return this->mask;
}

void Ppu::set_status_flag(StatusFlag flag, bool on) {
    if(on)
        this->status |= (0b1 << static_cast<int>(flag));
    else
        this->status &= ~(0b1 << static_cast<int>(flag));
}

void Ppu::write_status(uint8_t value) {
    this->status = value;
}

bool Ppu::get_status_flag(StatusFlag flag) {
    return this->status & (0b1 << static_cast<int>(flag));
}

uint8_t Ppu::read_status() {
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
    return this->address;
}

void Ppu::write_data(uint8_t value) {
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