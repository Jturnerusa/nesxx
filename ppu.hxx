#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <array>
#include "config.hxx"

//Forward declaration
class Bus;
class Frame;

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;

const int PPU_CONTROLLER = 0x2000;
const int PPU_MASK = 0x2001;
const int PPU_STATUS = 0x2002;
const int PPU_OAM_ADDRESS = 0x2003;
const int PPU_OAM_DATA = 0x2004;
const int PPU_SCROLL = 0x2005;
const int PPU_ADDRESS = 0x2006;
const int PPU_DATA = 0x2007;
const int PPU_OAM_DMA = 0x4014;

const int OAM_SIZE = 256;
const int PALLETE_TABLE_SIZE = 32;


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

class ScreenPosition {
public:
    ScreenPosition(int x, int y):x(x),y(y) {};
    const int x, y;
};

class Tile {
private:
    const std::array<uint8_t, 16> data;
public:
    Tile(std::array<uint8_t, 16>);
    int get_pixel(int, int);
    static ScreenPosition get_screen_position(int);
    static int get_attribute_table_index(int);
    static int get_attribute_table_quadrant(int);
};

class TileSlice {
private:
    const uint8_t bitplane_a, bitplane_b;
public:
    TileSlice(uint8_t, uint8_t);
};

class Sprite {
private:
  const std::array<uint8_t, 4> sprite_data;
  const std::array<uint8_t, 16> tile_data;
public:
  Sprite(std::array<uint8_t, 4>, std::array<uint8_t, 16>);
  int get_pixel(int, int);
  ScreenPosition get_screen_position();
  int get_attribute_table_index();
  static int get_pattern_table_index(std::array<uint8_t, 4>);
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
    const std::array<uint8_t, PALLETE_TABLE_SIZE> data;
public:
    FramePallete(std::array<uint8_t, PALLETE_TABLE_SIZE>);
    int get_backround_color_index(int, int);
    int get_sprite_color_index(int, int);
};

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

enum class MaskFlag {
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

class Ppu {
private:
    uint8_t controller, mask, status, oam_dma, oam_address, data, data_buffer;
    uint16_t scroll, address;
    bool address_io_in_progress, scroll_io_in_progress;
    std::array<uint8_t, OAM_SIZE> oam_data;
    std::array<uint8_t, PALLETE_TABLE_SIZE> pallete_ram;
    int scanline, pixel;
    uint64_t cycles, frame_count;
    Bus *bus;
    Frame *frame;
    int get_nametable();
    int get_sprite_pattern_table();
    int get_background_pattern_table();
    Tile get_tile(int);
    Sprite get_sprite(int); 
    FramePallete get_frame_pallete();
    AttributeTable get_attribute_table(int, int);
public:
    Ppu();
    void render_chrrom();
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
    void write_oam_data(uint8_t);
    uint8_t read_oam_data();
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
    void tick(int);
    int get_scanline() {return this->scanline;};
};

#ifdef UNITTEST

void run_ppu_tests();

#endif

#endif
