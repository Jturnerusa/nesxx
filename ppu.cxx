#include "ppu.hxx"
#include "bus.hxx"

void Ppu::set_controller_flag(ControllerFlag flag, bool on) {
    if(on) {
        switch (flag) {
            case ControllerFlag::base_nametable_address_1:
                this->controller |= 0b1;
                break;
            case ControllerFlag::base_nametable_address_2:
                this->controller |= 0b10;
                break;
            case ControllerFlag::vram_increment:
                this->controller |= 0b100;
                break;
            case ControllerFlag::sprite_pattern_table_address:
                this->controller |= 0b1000;
                break;
            case ControllerFlag::backround_pattern_table_address:
                this->controller |= 0b10000;
                break;
            case ControllerFlag::sprite_size:
                this->controller |= 0b100000;
                break;
            case ControllerFlag::ppu_master_slave_select:
                this->controller |= 0b1000000;
                break;
            case ControllerFlag::generate_nmi_on_vblank:
                this->controller |= 0b10000000;
                break;
        }
    }
    else {
        switch (flag) {
            case ControllerFlag::base_nametable_address_1:
                this->controller &= 0b11111110;
                break;
            case ControllerFlag::base_nametable_address_2:
                this->controller &= 0b11111101;
                break;
            case ControllerFlag::vram_increment:
                this->controller &= 0b11111011;
                break;
            case ControllerFlag::sprite_pattern_table_address:
                this->controller &= 0b11110111;
                break;
            case ControllerFlag::backround_pattern_table_address:
                this->controller &= 0b11101111;
                break;
            case ControllerFlag::sprite_size:
                this->controller &= 0b11011111;
                break;
            case ControllerFlag::ppu_master_slave_select:
                this->controller &= 0b10111111;
                break;
            case ControllerFlag::generate_nmi_on_vblank:
                this->controller &= 0b01111111;
                break;
        }
    }
}

void Ppu::write_controller(uint8_t value) {
    this->controller = value;
}

bool Ppu::get_controller_flag(ControllerFlag flag) {
    switch (flag) {
        case ControllerFlag::base_nametable_address_1:
            return this->controller & 0b1;
        case ControllerFlag::base_nametable_address_2:
            return this->controller & 0b10;
        case ControllerFlag::vram_increment:
            return this->controller & 0b100;
        case ControllerFlag::sprite_pattern_table_address:
            return this->controller & 0b1000;
        case ControllerFlag::backround_pattern_table_address:
            return this->controller & 0b10000;
        case ControllerFlag::sprite_size:
            return this->controller & 0b100000;
        case ControllerFlag::ppu_master_slave_select:
            return this->controller & 0b1000000;
        case ControllerFlag::generate_nmi_on_vblank:
            return this->controller & 0b10000000;
    }
}

uint8_t Ppu::read_controller() {
    return this->controller;
}

void Ppu::set_mask_flag(MaskFlag flag, bool on) {
    if(on) {
        switch (flag) {
            case MaskFlag::greyscale:
                this->mask |= 0b1;
                break;
            case MaskFlag::show_leftmost_backround:
                this->mask |= 0b10;
                break;
            case MaskFlag::show_leftmost_sprites:
                this->mask |= 0b100;
                break;
            case MaskFlag::show_backround:
                this->mask |= 0b1000;
                break;
            case MaskFlag::show_sprites:
                this->mask |= 0b10000;
                break;
            case MaskFlag::emphasize_red:
                this->mask |= 0b100000;
                break;
            case MaskFlag::emphasize_green:
                this->mask |= 0b1000000;
                break;
            case MaskFlag::emphasize_blue:
                this->mask |= 0b10000000;
                break;
        }
    }
    else {
        switch (flag) {
            case MaskFlag::greyscale:
                this->mask &= 0b11111110;
                break;
            case MaskFlag::show_leftmost_backround:
                this->mask &= 0b11111101;
                break;
            case MaskFlag::show_leftmost_sprites:
                this->mask &= 0b11111011;
                break;
            case MaskFlag::show_backround:
                this->mask &= 0b11110111;
                break;
            case MaskFlag::show_sprites:
                this->mask &= 0b11101111;
                break;
            case MaskFlag::emphasize_red:
                this->mask &= 0b11011111;
                break;
            case MaskFlag::emphasize_green:
                this->mask &= 0b10111111;
                break;
            case MaskFlag::emphasize_blue:
                this->mask &= 0b01111111;
                break;
        }
    }
}

void Ppu::write_mask(uint8_t value) {
    this->mask = value;
}

bool Ppu::get_mask_flag(MaskFlag flag) {
    switch (flag) {
        case MaskFlag::greyscale:
            return this->mask & 0b1;
        case MaskFlag::show_leftmost_backround:
            return this->mask & 0b10;
        case MaskFlag::show_leftmost_sprites:
            return this->mask & 0b100;
        case MaskFlag::show_backround:
            return this->mask & 0b1000;
        case MaskFlag::show_sprites:
            return this->mask & 0b10000;
        case MaskFlag::emphasize_red:
            return this->mask & 0b100000;
        case MaskFlag::emphasize_green:
            return this->mask & 0b1000000;
        case MaskFlag::emphasize_blue:
            return this->mask & 0b10000000;
    }
}

uint8_t Ppu::read_mask() {
    return this->mask;
}

void Ppu::set_status_flag(StatusFlag flag, bool on) {
    if(on) {
        switch (flag) {
            case StatusFlag::sprite_overflow:
                this->status |= 0b00100000;
                break;
            case StatusFlag::sprite_0_collision:
                this->status |= 0b01000000;
                break;
            case StatusFlag::vblank:
                this->status |= 0b10000000;
                break;
        }
    }
    else {
        switch (flag) {
            case StatusFlag::sprite_overflow:
            case StatusFlag::sprite_0_collision:
            case StatusFlag::vblank:
        }
    }
}