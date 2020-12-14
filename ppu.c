#include "ppu.h"

void init_ppu(struct PPU *self) {
    self->controller = 0;
    self->mask = 0;
    self->status = 0;
    self->oam_address = 0;
    sekf->oam_data = 0;
    self->scroll = 0;
    self->scroll_io_in_progress;
    self->address = 0;
    self->address_io_in_progess = 0;
    self->data = 0;
    self->data_buffer = 0;
    for (int i = 0; i < PPU_RAMSIZE; i++) {
        self->ram[i] = 0;
    }
    for (int i = 0; i < OAM_RAMSIZE; i++) {
        self->oam_ram[i] = 0;
    }
    self->nmi_interrupt = 0;
    self->vblank = 0;
    self->cycles = 0;
    self->scanline = 0;
    for (int x = 0; x < SCREEN_WIDTH; x++) {
        for (int y = 0; y < SCREEN_HEIGHT; y++) {
            self->frame_data[x][y] = 0;
        }
    }
}

uint8_t *access_ram(struct PPU *self, uint16_t address) {
    return &self->ram[address];
}

void write_controller(struct PPU *self, uint8_t value) {
    self->controller = value;
}

uint8_t read_controller_base_nametable_address(struct PPU *self) {
    return self->controller & 0b11;
}

int read_controller_vram_address_increment(struct PPU *self) {
    return self->controller & 0b100;
}

int read_controller_sprite_pattern_table_address(struct PPU *self) {
    return self->controller & 0b1000;
}

int read_controller_backround_pattern_table_address(struct PPU *self) {
    return self->controller & 0b10000;
}

int read_controller_sprite_size(struct PPU *self) {
    return self->controller & 0b100000;
}

int read_controller_ppu_master_slave_select(struct PPU *self) {
    return self->controller & 0b1000000;
}

int read_controller_generate_nmi_on_vbi(struct PPU *self) {
    return self->controller & 0b10000000;
}

void write_mask(struct PPU *self, uint8_t value) {
    self->mask = value;
}

int read_mask_greyscale(struct PPU *self) {
    return self->mask & 0b1;
}

int read_mask_show_leftmost_backround(struct PPU *self) {
    return self->mask & 0b10;
}

int read_mask_show_leftmost_sprites(struct PPU *self) {
    return self->mask & 0b100;
}

int read_mask_show_backround(struct PPU *self) {
    return self->mask & 0b1000;
}

int read_mask_show_sprites(struct PPU *self) {
    return self->mask & 0b10000;
}

int read_mask_emphasize_red(struct PPU *self) {
    return self->mask & 0b00100000;
}

int read_mask_emphasize_green(struct PPU *self) {
    return self->mask & 0b01000000;
}

int read_mask_emphasize_blue(struct PPU *self) {
    return self->mask & 0b10000000;
}

int read_status_sprite_overflow(struct PPU *self) {
    return self->status & 0b00100000;
}

void write_status_sprite_overflow(struct PPU *self) {
    self->status |= 0b00100000;
}

int read_status_sprite_collision(struct PPU *self) {
    return self->status & 0b01000000;
}

void write_status_sprite_collision(struct PPU *self) {
    self->status |= 0b01000000;
}

int read_status_vblank_started(struct PPU *self) {
    return self->status & 0b10000000;
}

void write_status_vblank_started(struct PPU *self) {
    self->status |= 0b10000000;
}

void write_oam_address(struct PPU *self, uint8_t value) {
    self->oam_address = value;
}

void write_oam_data(struct PPU *self, uint8_t value) {
    self->oam_address++;
    self->oam_data = value;
}

void write_scroll(struct PPU *self, uint8_t value) {
    if (self->scroll_io_in_progress == 0) {
        self->scroll_io_in_progress = 1;
        self->scroll = value << 8;
    }
    else {
        self->scroll_io_in_progress = 0;
        self->scroll |= value;
    }
}

void write_register_address(struct PPU *self, uint8_t data) {
    if (self->address_write_in_progess == 0) {
        self->address = data << 8;
        self->address_write_in_progess = 1;
    }
    else {
        self->address |= data;
        self->address_write_in_progess = 0;
    }
}

uint8_t read_data(struct PPU *self) {
    if (self->address <= 0x3eff) {
        self->data = self->data_buffer;
        self->data_buffer = *access_ram(self, self->address);
        self->address++;
        return self->data;
    }
    else {
        self->data = *access_ram(self, self->address);
        self->data_buffer = *access_ram(self, self->address);
        self->address++;
        return self->data;        
    }
}

void write_data(struct PPU *self, uint8_t data) {
    *access_ram(self, self->address) = data;
    self->address++;
}

void sync_cpu_write_io(struct PPU *self, uint16_t address_accessed, uint8_t data) {
    switch(address_accessed) {
        case 0x2000:
            write_controller(self, data);
            break;
        case 0x2001:
            write_mask(self, data);
            break;
        case 0x2003:
            write_oam_address(self, data);
            break;
        case 0x2004:
            write_oam_data(self, data);
            break;
        case 0x2005:
            write_scroll(self, data);
            break;
        case 0x2006:
            write_address(self, data);
            break;
        case 0x20007:
            write_data(self, data);
            break;
    }
}



















