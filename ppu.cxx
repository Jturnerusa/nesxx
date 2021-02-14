#include "ppu.hxx"
#include "bus.hxx"
#include "frame.hxx"
#ifdef PPU_DEBUG_OUTPUT
#include <iostream>
#endif

TileSlice::TileSlice(uint8_t bitplane_a, uint8_t bitplane_b):bitplane_a(bitplane_a),
                                                             bitplane_b(bitplane_b) {};

int TileSlice::get_pixel(int x, bool flip) {
    if(flip) {
        int pixel_a = ((this->bitplane_a >> x) & 0b1);
        int pixel_b = ((this->bitplane_b >> x) & 0b1);
        return ((pixel_b << 1) | pixel_a);
    }
    else {
        int pixel_a = ((this->bitplane_a >> (7 - x)) & 0b1);
        int pixel_b = ((this->bitplane_b >> (7 - x)) & 0b1);
        return ((pixel_b << 1) | pixel_a);
    }
}

int BackgroundTile::get_tile_index(int x, int y) {
    /*This allows us look up what byte in the nametable we need to fetch for the tile slice at a given position,
     * amount other things. */
    int tile_index_x = x / 8;
    int tile_index_y = (y / 8) * 32;
    return tile_index_x + tile_index_y;
}

int BackgroundTile::get_attribute_table_index(int tile_index) {
    int block_x = (tile_index / 4) % 8;
    int block_y = (tile_index / (32 * 4)) * 8;
    return block_x + block_y;
}

int BackgroundTile::get_attribute_table_quadrant(int tile_index) {
    int quadrant_x = tile_index % 4;
    int quadrant_y = (tile_index / 32) % 4;
    int q = 0;
    if (quadrant_x >= 2) {
        q += 1;
    }
    if (quadrant_y >= 2) {
        q += 2;
    }
    return q;
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

FramePallete::FramePallete(const std::array<uint8_t, PALLETE_SIZE>& pallete_data):data(pallete_data) {};

int FramePallete::get_backround_color_index(int pallete, int index) {
    int color_group_offset = 4 * pallete;
    return this->data.at(color_group_offset + index);
}

int FramePallete::get_sprite_color_index(int pallete, int index) {
    int color_group_offset = 16 + (pallete * 4);
    return this->data.at(color_group_offset + index);
}

Sprite::Sprite(uint8_t x, uint8_t y, uint8_t pattern_table_index, uint8_t attribute):
              x(x),y(y + 1),pattern_table_index(pattern_table_index),attribute(attribute) {};

int Sprite::get_x_position() {
    return this->x;
}

int Sprite::get_y_position() {
    return this->y;
}

int Sprite::get_pattern_table_index() {
    return this->pattern_table_index;
}

int Sprite::get_attribute(Attribute attribute) {
    return this->attribute & static_cast<unsigned int>(attribute);
}

bool Sprite::is_visible_on_scanline(int scanline) {
    if(this->y <= 1) {
        return false;
    }
    int delta = scanline - this->y;
    if(delta > 0 && delta < 8) {
        return true;
    }
    else {
        return false;
    }
}

int Sprite::get_visible_slice(int scanline) {
    if(this->get_attribute(Attribute::vertical_flip)) {
        return 7 - (scanline - this->y);
    }
    else {
        return scanline - this->y;
    }
}

Ppu::Ppu() {
    this->controller  = 0;
    this->mask        = 0;
    this->status      = 0;
    this->oam_address = 0;
    this->oam.fill(0);
    this->scroll  = 0;
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
    if (on) {
        this->controller |= static_cast<unsigned int>(flag);
    }
    else {
        this->controller &= ~static_cast<unsigned int>(flag);
    }
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
    if (on) {
        this->mask |= static_cast<unsigned int>(flag);
    }
    else {
        this->mask &= ~static_cast<unsigned int>(flag);
    }
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
    if (on) {
        this->status |= static_cast<unsigned int>(flag);
    }
    else {
        this->status &= ~static_cast<unsigned int>(flag);
    }
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
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to oam address value "
              << std::hex
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->oam_address = value;
}

uint8_t Ppu::read_oam_address() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading oam address value "
              << std::hex
              << static_cast<unsigned int>(this->oam_address)
              << std::endl;
    #endif
    return this->oam_address;
}

void Ppu::write_oam(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Writing to oam directly to address "
              << std::hex
              << static_cast<unsigned int>(this->oam_address)
              << " value "
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->oam.at(this->oam_address) = value;
    this->oam_address++;
}

uint8_t Ppu::read_oam() {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Reading from oam directly from address "
              << std::hex
              << static_cast<unsigned int>(this->oam_address)
              << " value "
              << static_cast<unsigned int>(this->oam.at(this->oam_address))
              << std::endl;
    #endif
    uint8_t value = this->oam.at(this->oam_address);
    if (!this->get_status_flag(StatusFlag::vblank)) {
        this->oam_address++;
    }
    return value;
}

void Ppu::set_scroll_position(ScrollPosition position, uint8_t value) {
    if (position == ScrollPosition::horizontal) {
        this->scroll = value | (this->scroll >> 8) << 8;
    }
    else {
        this->scroll = value << 8 | (this->scroll & 0xff);
    }
}

void Ppu::write_scroll(uint8_t value) {
    if (!this->scroll_io_in_progress) {
        this->scroll_io_in_progress = true;
        this->scroll = value << 8;
    }
    else {
        this->scroll_io_in_progress = false;
        this->scroll |= value;
    }
}

uint8_t Ppu::get_scroll_position(ScrollPosition position) {
    if (position == ScrollPosition::horizontal)
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
    if (!this->address_io_in_progress) {
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
    if (this->get_controller_flag(ControllerFlag::vram_increment)) {
        this->address += 32;
    }
    else {
        this->address++;
    }
}

uint8_t Ppu::read_data() {
    if (this->address <= Bus::NAMETABLE_START) {
        this->data        = this->data_buffer;
        this->data_buffer = this->bus->read_vram(this->address);
    }
    else {
        this->data        = this->bus->read_vram(this->address);
        this->data_buffer = this->bus->read_vram(this->address);
    }
    if (this->get_controller_flag(ControllerFlag::vram_increment)) {
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
    if (this->get_status_flag(StatusFlag::vblank) &&
        this->get_controller_flag(ControllerFlag::generate_nmi_on_vblank))
    {
        return true;
    }
    else {
        return false;
    }
}

int Ppu::get_nametable() {
    int index = 0;
    if (this->get_controller_flag(ControllerFlag::base_nametable_address_1)) {
        index += 1;
    }
    if (this->get_controller_flag(ControllerFlag::base_nametable_address_2)) {
        index += 2;
    }
    return index;
}

int Ppu::get_background_pattern_table() {
    if (this->get_controller_flag(ControllerFlag::background_pattern_table_address)) {
        return 1;
    }
    else {
        return 0;
    }
}

int Ppu::get_sprite_pattern_table() {
    if (this->get_controller_flag(ControllerFlag::sprite_pattern_table_address)) {
        return 1;
    }
    else {
        return 0;
    }
}

int Ppu::get_pattern_table_index_from_nametable(int tile_index, int nametable) {
    int nametable_address = Bus::NAMETABLE_START + (Bus::NAMETABLE_SIZE * nametable) + tile_index;
    return this->bus->read_vram(nametable_address);
}

TileSlice Ppu::get_tile_slice(int pattern_table_index, int pattern_table, int slice) {
    int pattern_table_address = (Bus::PATTERN_TABLE_SIZE * pattern_table) + (pattern_table_index * 16) + slice;
    uint8_t bitplane_a = this->bus->read_vram(pattern_table_address);
    uint8_t bitplane_b = this->bus->read_vram(pattern_table_address + 8);
    return TileSlice(bitplane_a, bitplane_b);
}

AttributeTable Ppu::get_attribute_table(int i, int nametable) {
    int attribute_table_address = (Bus::NAMETABLE_START  + (Bus::NAMETABLE_SIZE * nametable)) + 960 + i;
    return AttributeTable(this->bus->read_vram(attribute_table_address));
}

FramePallete Ppu::get_frame_pallete() {
    return FramePallete(this->pallete_ram);
}

Sprite Ppu::get_sprite(int sprite_index) {
    sprite_index *= 4;
    uint8_t y = this->oam.at(sprite_index);
    uint8_t pattern_table_index = this->oam.at(sprite_index + 1);
    uint8_t attribute = this->oam.at(sprite_index + 2);
    uint8_t x = this->oam.at(sprite_index + 3);
    return Sprite(x, y, pattern_table_index, attribute);
}

void Ppu::receive_oam_dma(uint8_t value) {
    #ifdef PPU_DEBUG_OUTPUT
    std::cout << "Receiving oam dma, writing value "
              << std::hex
              << static_cast<unsigned int>(value)
              << " to address "
              << static_cast<unsigned int>(this->oam_address)
              << std::endl;
    #endif
    this->oam.at(this->oam_address) = value;
    this->oam_address++;
}

void Ppu::render_background() {
    int pixel = 0;
    auto frame_pallete = this->get_frame_pallete();
    for(int t = 0; t < 32; t++) {
        int tile_index = BackgroundTile::get_tile_index(pixel, this->scanline);
        int pattern_table_index = this->get_pattern_table_index_from_nametable(tile_index, this->get_nametable());
        int attribute_table_index = BackgroundTile::get_attribute_table_index(tile_index);
        int attribute_table_quadrant = BackgroundTile::get_attribute_table_quadrant(tile_index);
        auto tile_slice = this->get_tile_slice(pattern_table_index,
                                               this->get_background_pattern_table(),
                                               this->scanline % 8);
        auto attribute_table = this->get_attribute_table(attribute_table_index, this->get_nametable());
        for(int x = 0; x < 8; x++) {
            int pixel_value = tile_slice.get_pixel(x);
            int pallete = attribute_table.get_pallete(attribute_table_quadrant);
            int system_pallete_index = frame_pallete.get_backround_color_index(pallete, pixel_value);
            uint32_t color = SYSTEM_PALLETE.at(system_pallete_index);
            this->frame->set_pixel(pixel, this->scanline, color);
            pixel++;
        }
    }
}


void Ppu::render_sprites() {
    int sprites_rendered = 0;
    auto frame_pallete = this->get_frame_pallete();
    for(int sprite_index = 63; sprite_index >= 0; sprite_index--) {
        auto sprite = this->get_sprite(sprite_index);
        if(sprite.is_visible_on_scanline(this->scanline)) {
            if(sprites_rendered > 7) {
                this->set_status_flag(StatusFlag::sprite_overflow, true);
                break;
            }
            auto tile_slice = this->get_tile_slice(sprite.get_pattern_table_index(),
                                                   this->get_sprite_pattern_table(),
                                                   sprite.get_visible_slice(this->scanline));
            int pallete = sprite.get_attribute(Sprite::Attribute::pallete);
            for(int x = 0; x < 8 && x + sprite.get_x_position() < this->frame->WIDTH; x++) {
                int pixel_value = tile_slice.get_pixel(x, sprite.get_attribute(Sprite::Attribute::horizontal_flip));
                int system_pallete_index = frame_pallete.get_sprite_color_index(pallete, pixel_value);
                uint32_t color = SYSTEM_PALLETE.at(system_pallete_index);
                if(color != 0) {
                    if(!sprite.get_attribute(Sprite::Attribute::priority)) {
                        this->frame->set_pixel(x + sprite.get_x_position(), this->scanline, color);
                    }
                    else {
                        if(this->frame->get_pixel(x + sprite.get_x_position(), this->scanline) == 0) {
                            this->frame->set_pixel(x + sprite.get_x_position(), this->scanline, color);
                        }
                    }
                }
            }
        }
        else {
            continue;
        }
    }
}

void Ppu::render_scanline() {
    if(this->scanline < 240) {
        if(this->get_mask_flag(MaskFlag::show_backround)) {
            this->render_background();
        }
        if(this->get_mask_flag(MaskFlag::show_sprites)) {
            this->render_sprites();
        }
    }
    this->scanline++;
    if(this->scanline == 240) {
        this->set_status_flag(StatusFlag::vblank, true);
    }
    if(this->scanline == 261) {
        this->set_status_flag(StatusFlag::vblank, false);
        this->scanline = 0;
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
