#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <array>

//Forward declaration
class Bus;
class Frame;

const int PPU_CONTROLLER = 0x2000;
const int PPU_MASK = 0x2001;
const int PPU_STATUS = 0x2002;
const int PPU_OAM_ADDRESS = 0x2003;
const int PPU_OAM_DATA = 0x2004;
const int PPU_SCROLL = 0x2005;
const int PPU_ADDRESS = 0x2006;
const int PPU_DATA = 0x2007;
const int PPU_OAM_DMA = 0x4014;

const int PATTERN_TABLE_SIZE = 0x1000;
const int NAMETABLE_SIZE = 0x0400;

const int OAM_SIZE = 256;
const int PALLETE_TABLE_SIZE = 32;

enum class ControllerFlag {
    base_nametable_address_1        = 0b1,
    base_nametable_address_2        = 0b10,
    vram_increment                  = 0b100,
    sprite_pattern_table_address    = 0b1000,
    backround_pattern_table_address = 0b10000,
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
    uint8_t controller;
    uint8_t mask;
    uint8_t status;
    uint8_t oam_address;
    uint16_t scroll;
    uint16_t address;
    uint8_t data;
    uint8_t data_buffer;
    uint8_t oam_dma;
    bool address_io_in_progress;
    bool scroll_io_in_progress;
    std::array<uint8_t, OAM_SIZE> oam_data;
    std::array<uint8_t, PALLETE_TABLE_SIZE> pallete_ram;
    int scanline;
    int pixel;
    uint64_t cycles, frame_count;
    Bus *bus;
    Frame *frame;
    int base_nametable_index();
    int base_pattern_table_index();
public:
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

#endif
