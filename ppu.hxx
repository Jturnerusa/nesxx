#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <array>
#include "config.hxx"

//Forward declaration
class Bus;
class Frame;

class BackgroundTile {
public:
    static int get_tile_index(int, int);
    static int get_attribute_table_index(int);
    static int get_attribute_table_quadrant(int);
};

class TileSlice {
private:
    const uint8_t bitplane_a, bitplane_b;
public:
    TileSlice(uint8_t, uint8_t);
    int get_pixel(int, bool flip = false);
};

class AttributeTable {
private:
    const uint8_t data;
public:
    AttributeTable(uint8_t);
    int get_pallete(int);
};

class FramePallete {
private:
    static const int PALLETE_SIZE = 32;
    const std::array<uint8_t, PALLETE_SIZE>& data;
public:
    FramePallete(const std::array<uint8_t, PALLETE_SIZE>&);
    int get_backround_color_index(int, int);
    int get_sprite_color_index(int, int);
};

class Sprite {
private:
    const uint8_t x, y, pattern_table_index, attribute;
public:
    enum class Attribute {
        vertical_flip   = 0b10000000,
        horizontal_flip = 0b1000000,
        priority        = 0b100000,
        pallete         = 0b11
    };
    Sprite(uint8_t, uint8_t, uint8_t, uint8_t);
    int get_x_position();
    int get_y_position();
    int get_pattern_table_index();
    int get_attribute(Attribute attribute);
    bool is_visible_on_scanline(int);
    int get_visible_slice(int);
};

class Ppu {
public:
    static const int PPU_CONTROLLER = 0x2000;
    static const int PPU_MASK = 0x2001;
    static const int PPU_STATUS = 0x2002;
    static const int PPU_OAM_ADDRESS = 0x2003;
    static const int PPU_OAM = 0x2004;
    static const int PPU_SCROLL = 0x2005;
    static const int PPU_ADDRESS = 0x2006;
    static const int PPU_DATA = 0x2007;
    static const int PPU_OAM_DMA = 0x4014;
    static const int OAM_SIZE = 256;
    static const int PALLETE_TABLE_SIZE = 32;
private:
    enum class ControllerFlag {
        base_nametable_address_1        = 0b1,
        base_nametable_address_2        = 0b10,
        vram_increment                  = 0b100,
        sprite_pattern_table_address    = 0b1000,
        background_pattern_table_address = 0b10000,
        sprite_size                     = 0b100000,
        ppu_master_slave_select         = 0b1000000,
        generate_nmi_on_vblank          = 0b10000000
    };
    enum class MaskFlag{
        greyscale               = 0b1,
        show_leftmost_backround = 0b10,
        show_leftmost_sprites   = 0b100,
        show_backround          = 0b1000,
        show_sprites            = 0b10000,
        emphasize_red           = 0b100000,
        emphasize_green         = 0b1000000,
        emphasize_blue          = 0b10000000
    };
    enum class StatusFlag {
        sprite_overflow    = 0b100000,
        sprite_0_collision = 0b1000000,
        vblank             = 0b10000000
    };
    enum class ScrollPosition {
        horizontal,
        vertical
    };
private:
    const std::array<uint32_t , 64> SYSTEM_PALLETE{0x656565, 0x002d69, 0x131f7f, 0x3c137c, 0x600b62, 0x730a37, 0x710f07,
                                                          0x5a1a00, 0x342800, 0x0b3400, 0x003c00, 0x003d10, 0x003840, 0x000000,
                                                          0x000000, 0x000000, 0xaeaeae, 0x0f63b3, 0x4051d0, 0x7841cc, 0xa736a9,
                                                          0xc03470, 0xbd3c30, 0x9f4a00, 0x6d5c00, 0x366d00, 0x077704, 0x00793d,
                                                          0x00727d, 0x000000, 0x000000, 0x000000, 0xfefeff, 0x5db3ff, 0x8fa1ff,
                                                          0xc890ff, 0xf785fa, 0xff83c0, 0xff8b7f, 0xef9a49, 0xbdac2c, 0x85bc2f,
                                                          0x55c753, 0x3cc98c, 0x3ec2cd, 0x4e4e4e, 0x000000, 0x000000, 0xfefeff,
                                                          0xbcdfff, 0xd1d8ff, 0xe8d1ff, 0xfbcdfd, 0xffcce5, 0xffcfca, 0xf8d5b4,
                                                          0xe4dca8, 0xcce3a9, 0xb9e8b8, 0xaee8d0, 0xafe5ea, 0xb6b6b6, 0x000000,
                                                          0x000000};
    uint8_t controller, mask, status, oam_address, data, data_buffer;
    uint16_t scroll, address;
    bool address_io_in_progress, scroll_io_in_progress;
    std::array<uint8_t, 256> oam;
    std::array<uint8_t, 32> pallete_ram;
    int scanline;
    uint64_t cycles;
    Bus *bus;
    Frame *frame;
    int get_nametable();
    int get_sprite_pattern_table();
    int get_background_pattern_table();
    int get_pixel_from_tile(int, int, int, int);
    int get_pallete_from_attribute_table(int, int, int);
    int get_backround_color_from_frame_pallete(int, int);
    int get_pattern_table_index_from_nametable(int, int);
    TileSlice get_tile_slice(int, int, int);
    AttributeTable get_attribute_table(int, int);
    FramePallete get_frame_pallete();
    Sprite get_sprite(int);
    void render_background();
    void render_sprites();
public:
    Ppu();
    void reset();
    void connect_bus(Bus*);
    void connect_frame(Frame*);
    void set_controller_flag(ControllerFlag, bool);
    void write_controller(uint8_t);
    bool get_controller_flag(ControllerFlag);
    uint8_t read_controller();
    void set_mask_flag(MaskFlag, bool);
    void write_mask(uint8_t);
    bool get_mask_flag(MaskFlag);
    uint8_t read_mask();
    void write_oam_address(uint8_t);
    uint8_t read_oam_address();
    void write_oam(uint8_t);
    uint8_t read_oam();
    void set_status_flag(StatusFlag, bool);
    void write_status(uint8_t);
    bool get_status_flag(StatusFlag);
    uint8_t read_status();
    void set_scroll_position(ScrollPosition, uint8_t);
    void write_scroll(uint8_t);
    uint8_t get_scroll_position(ScrollPosition);
    uint16_t read_scroll();
    void write_address(uint8_t);
    uint16_t read_address();
    void write_data(uint8_t);
    uint8_t read_data();
    void write_pallete_ram(uint16_t, uint8_t);
    uint8_t read_pallete_ram(uint16_t);
    bool poll_nmi_interrupt();
    void render_scanline();
    void catch_up(int);
    int get_scanline() {return this->scanline;};
    void receive_oam_dma(uint8_t);
};

#ifdef UNITTEST

void run_ppu_tests();

#endif

#endif