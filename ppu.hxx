#ifndef PPU_H
#define PPU_H
#include <cstdint>
#include <array>

//Forward declaration
class bus;

const int PPU_CONTROLLER = 0x2000;
const int PPU_MASK = 0x2001;
const int PPU_STATUS = 0x2002;
const int PPU_OAM_ADDRESS = 0x2003;
const int PPU_OAM_DATA = 0x2004;
const int PPU_SCROLL = 0x2005;
const int PPU_ADDRESS = 0x2006;
const int PPU_DATA = 0x2007;
const int PPU_OAM_DMA = 0x4014;

enum ControllerFlag {
    base_nametable_address_1 = 0,
    base_nametable_address_2 = 1,
    vram_increment = 2,
    sprite_pattern_table_address = 3,
    backround_pattern_table_address = 4,
    sprite_size = 5,
    ppu_master_slave_select = 6,
    generate_nmi_on_vblank = 7
};

enum MaskFlag {
    greyscale,
    show_leftmost_backround,
    show_leftmost_sprites,
    show_backround,
    show_sprites,
    emphasize_red,
    emphasize_green,
    emphasize_blue
};

enum StatusFlag {
    sprite_overflow,
    sprite_0_collision,
    vblank
};

enum ScrollPosition {
    horizontal,
    vertical
};

class Ppu {
public:
    void set_controller_flag(ControllerFlag, bool);
    void write_controller(uint8_t);
    bool get_controller_flag(ControllerFlag);
    uint8_t read_controller();

    void set_mask_flag(MaskFlag, bool);
    void write_mask(uint8_t);
    bool get_mask_flag(MaskFlag);
    uint8_t read_mask();

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

private:
    uint8_t controller;
    uint8_t mask;
    uint8_t status;
    uint8_t oam_address;
    uint8_t oam_data;
    uint16_t scroll;
    uint16_t address;
    uint8_t data;
    uint8_t oam_dma;
    bool address_io_in_progress;
    bool scroll_io_in_progress;
};

#endif
