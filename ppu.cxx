#include <iostream>
#include "ppu.hxx"
#include "bus.hxx"
#include "frame.hxx"

Tile::Tile(std::array<uint8_t, 16> data):data(data) {};

int Tile::get_pixel(int x, int y) {
    uint8_t tile_slice_a = this->data.at(y);
    uint8_t tile_slice_b = this->data.at(y + 8);
    int pixel_value_a = (tile_slice_a >> (7 - x)) & 0b1;
    int pixel_value_b = (tile_slice_b >> (7 - x)) & 0b1;
    int color_value = (pixel_value_b << 1) | pixel_value_a;
    return color_value;
}

ScreenPosition Tile::get_screen_position(int tile_index) {
    int origin_x = (tile_index % 32) * 8;
    int origin_y = (tile_index / 32) * 8;
    ScreenPosition screen_position(origin_x, origin_y);
    return screen_position;
}

int Tile::get_attribute_table_index(int tile_index) {
    int block_x = (tile_index / 4) % 8;
    int block_y = (tile_index / (32 * 4)) * 8;
    return block_x + block_y;
}

int Tile::get_attribute_table_quadrant(int tile_index) {
    int quadrant_x = tile_index % 4;
    int quadrant_y = (tile_index / 32) % 4;
    int q = 0;
    if(quadrant_x >= 2) {
        q += 1;
    }
    if(quadrant_y >= 2) {
        q += 2;
    }
    return q;
}

Sprite::Sprite(std::array<uint8_t, 4> sprite_data, std::array<uint8_t, 16> tile_data):sprite_data(sprite_data),
                                                                                      tile_data(tile_data) {};

int Sprite::get_pixel(int x, int y) {
    uint8_t tile_slice_a = this->tile_data.at(y);
    uint8_t tile_slice_b = this->tile_data.at(y + 8);
    int pixel_value_a = (tile_slice_a >> (7 - x)) & 0b1;
    int pixel_value_b = (tile_slice_b >> (7 - x)) & 0b1;
    int color_value = (pixel_value_b << 1) | pixel_value_a;
    return color_value;
}

ScreenPosition Sprite::get_screen_position() {
    int x = this->sprite_data.at(0);
    int y = this->sprite_data.at(3);
    ScreenPosition screen_position(x, y);
    return screen_position;
}

int Sprite::get_attribute_table_index() {
    return this->sprite_data.at(2);
}

int Sprite::get_pattern_table_index(std::array<uint8_t, 4> sprite_data) {
    return sprite_data.at(1);
}

AttributeTable::AttributeTable(uint8_t data):data(data) {};

int AttributeTable::get_pallete(int quadrant) {
    switch (quadrant) {
        case 0:
            return this->data & 0b11;
        case 1:
            return (this->data >> 2) & 0b11;
        case 2:
            return (this->data >> 4) & 0b11;
        case 3:
            return (this->data >> 6) & 0b11;
        default:
            throw std::runtime_error("Quadrant must be in range 0-3.");
    }
}

FramePallete::FramePallete(std::array<uint8_t, PALLETE_TABLE_SIZE> pallete_data):data(pallete_data) {};

int FramePallete::get_backround_color_index(int pallete, int index) {
    int color_group_offset = 4 * pallete;
    return this->data.at(color_group_offset + index);
}

int FramePallete::get_sprite_color_index(int pallete, int index) {
    int color_group_offset = 16 + (pallete * 4);
    return this->data.at(color_group_offset + index);
}

Ppu::Ppu() {
    this->controller = 0;
    this->mask = 0;
    this->status = 0;
    this->oam_address = 0;
    this->oam_data.fill(0);
    this->scroll = 0;
    this->address = 0;
    this->pallete_ram.fill(0);
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
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing pallete ram at address "
              << std::hex
              << static_cast<unsigned int>(address)
              << " value "
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->pallete_ram.at(address) = value;
}

uint8_t Ppu::read_pallete_ram(uint16_t address) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading pallete ram at address "
              << std::hex
              << static_cast<unsigned int>(address)
              << std::endl;
    #endif
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

int Ppu::get_nametable() {
    int index = 0;
    if(this->get_controller_flag(ControllerFlag::base_nametable_address_1)){
        index += 1;
    }
    if(this->get_controller_flag(ControllerFlag::base_nametable_address_2)) {
        index += 2;
    }
    return index;
}

int Ppu::get_background_pattern_table() {
    if(this->get_controller_flag(ControllerFlag::background_pattern_table_address)) {
        return 1;
    }
    else {
		return 0;
	}
}

int Ppu::get_sprite_pattern_table() {
	if(this->get_controller_flag(ControllerFlag::sprite_pattern_table_address)) {
		return 1;
	}
	else {
		return 0;
	}
}

Tile Ppu::get_tile(int pattern_table_index) {
    std::array<uint8_t, 16> tile_data;
    pattern_table_index *= 16;
    pattern_table_index += PATTERN_TABLE_SIZE * this->get_background_pattern_table();
    for(int i = pattern_table_index, e = 0; i < pattern_table_index + 16; i++, e++) {
        tile_data.at(e) = this->bus->read_vram(i);
    }
    return Tile(tile_data);
}

Sprite Ppu::get_sprite(int sprite_index) {
	sprite_index *= 4;
	std::array<uint8_t, 4> sprite_data;
	std::array<uint8_t, 16> tile_data;
	for(int i = sprite_index, e = 0; i < sprite_index + 4; i++, e++) {
		sprite_data.at(e) = this->oam_data.at(i);
	}
	int pattern_table_index = Sprite::get_pattern_table_index(sprite_data) + (PATTERN_TABLE_SIZE * this->get_sprite_pattern_table());
	for(int i = pattern_table_index, e = 0; i < pattern_table_index + 16; i++, e++) {
		tile_data.at(e) = this->bus->read_vram(i);
	}
	return Sprite(sprite_data, tile_data);
}

AttributeTable Ppu::get_attribute_table(int i, int pattern_table_index) {
    int attribute_table_address = (NAMETABLE_START  + (NAMETABLE_SIZE * pattern_table_index)) + 960 + i;
    AttributeTable attribute_table(this->bus->read_vram(attribute_table_address));
    return attribute_table;
}

void Ppu::tick(int cycles) {
    for(int cycle = 0; cycle < cycles * 3; cycle++) {
        if(this->scanline == 240 & this->pixel == 256) {
            FramePallete frame_pallete(this->pallete_ram);
            int nametable_address = NAMETABLE_START + (NAMETABLE_SIZE * this->get_nametable());
            for(int nametable_index = nametable_address, i = 0; nametable_index < nametable_address + 960; nametable_index++, i++) {
                int pattern_table_index = this->bus->read_vram(nametable_index);
                auto tile = this->get_tile(pattern_table_index);
                auto attribute_table = this->get_attribute_table(Tile::get_attribute_table_index(i), this->get_background_pattern_table());
				int pallete_index = attribute_table.get_pallete(Tile::get_attribute_table_quadrant(i));
				auto screen_position = Tile::get_screen_position(i);
                for(int y = 0; y < 7; y++) {
                    for(int x = 0; x < 7; x++) {
                        int color_index = tile.get_pixel(x, y);
                        int system_pallete_index = frame_pallete.get_backround_color_index(pallete_index, color_index);
                        uint32_t color = SYSTEM_PALLETE.at(system_pallete_index);
                        frame->set_pixel(screen_position.x + x, screen_position.y + y, color);
                    }
                }
            }
            this->set_status_flag(StatusFlag::vblank, true);
        }
        if(this->pixel == 341) {
            this->pixel = 0;
            this->scanline++;
        }
        if(this->scanline == 262) {
            this->pixel = 0;
            this->scanline = 0;
            this->set_status_flag(StatusFlag::vblank, false);
        }
        this->pixel++;
    }
}

#ifdef UNITTEST

#include <cassert>

const std::array<uint8_t, 16> test_tile_data{0b10101010,
                                             0b11111111,
                                             0b01010101,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000,

                                             0b10101010,
                                             0b00001111,
                                             0b11111111,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000,
                                             0b00000000};

void test_tile_get_screen_position() {
    Tile tile(1, test_tile_data);
    auto screen_position = tile.get_screen_position();
    assert(screen_position.x == 8);
    assert(screen_position.y == 0);
    Tile tile2(32, test_tile_data);
    auto screen_position2 = tile2.get_screen_position();
    assert(screen_position2.x == 0);
    assert(screen_position2.y == 8);
    std::cout << "Tile get screen position test passed" << std::endl;
}

void test_tile_get_pixel() {
    Tile tile(1, test_tile_data);
    assert(tile.get_pixel(0, 0) == 0b11);
    assert(tile.get_pixel(0, 1) == 0b1);
    assert(tile.get_pixel(0, 2) == 0b10);
    assert(tile.get_pixel(7,7) == 0b0);
    std::cout << "Tile get pixel test passed" << std::endl;
}

void test_tile_get_attribute_table_index() {
    Tile tile(0, test_tile_data);
    assert(tile.get_attriubute_table_index() == 0);
    Tile tile2(31, test_tile_data);
    assert(tile2.get_attriubute_table_index() == 7);
    Tile tile3(32, test_tile_data);
    assert(tile3.get_attriubute_table_index() == 0);
    Tile tile4(128, test_tile_data);
    assert(tile4.get_attriubute_table_index() == 8);
    std::cout << "Tile get attribute table index test passed" << std::endl;
}

void test_tile_get_attribute_table_quadrant() {
    Tile tile(0, test_tile_data);
    assert(tile.get_attribute_table_quadrant() == 0);
    Tile tile2(2, test_tile_data);
    assert(tile2.get_attribute_table_quadrant() == 1);
    Tile tile3(65, test_tile_data);
    assert(tile3.get_attribute_table_quadrant() == 2);
    Tile tile4(66, test_tile_data);
    assert(tile4.get_attribute_table_quadrant() == 3);
    Tile tile5(132, test_tile_data);
    assert(tile5.get_attribute_table_quadrant() == 0);
    std::cout << "Tile get attribute table quadrant test passed" << std::endl;
}

void run_ppu_tests() {
    test_tile_get_screen_position();
    test_tile_get_pixel();
    test_tile_get_attribute_table_index();
    test_tile_get_attribute_table_quadrant();
}

#endif

/* 262
//scanlines per frame, scanline lasts for 341 cycles, scanline 241=vblank
void Ppu::tick(int cycles) {
    for(int cycle = 0; cycle < cycles * 3; cycle++) {
        if(this->scanline < 240 & this->pixel < 256) {
            int base_nametable_address = NAMETABLE_START + (NAMETABLE_SIZE * this->base_nametable_index());
            int nametable_x = this->pixel / 8;
            int nametable_y = (this->scanline / 8) * 32;
            int nametable_index = base_nametable_address + nametable_x + nametable_y;
            int pattern_table_index = this->bus->read_vram(nametable_index);
            auto tile = this->get_tile(pattern_table_index);
            int x = this->pixel % 8;
            int y = this->scanline % 8;
            if(tile.get_pixel(x, y)) {
                this->frame->set_pixel(this->pixel, this->scanline, Color::white);
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
            this->frame->clear();
        }
        this->pixel++;
    }
}

*/
