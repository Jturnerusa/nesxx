#include "ppu.h"

void init_ppu(struct PPU *self) {
    self->controller = 0;
    self->mask = 0;
    self->status = 0;
    self->oam_address = 0;
    sekf->oam_data = 0;
    self->scroll = 0;
    self->address = 0;
    self->address_io_in_progess = 0;
    self->data = 0;
    self->data_buffer = 0;
    self->oam_dma = 0;
    for (int i = 0; i < PPU_RAMSIZE; i++) {
        self->ram[i] = 0;
    }
    self->nmi_interrupt = 0;
}

uint8_t *access_ram(struct PPU *self, uint16_t address) {
    return &self->ram[address];
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

uint8_t read_register_data(struct PPU *self) {
    self->data = self->data_buffer;
    self->data_buffer = *access_ram(self, self->address);
    self->address++;
    return self->data;
}

void write_register_data(struct PPU *self, uint8_t data) {
    *access_ram(self, self->address) = data;
    self->address++;
}
