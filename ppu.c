#include "ppu.h"

void init_ppu(struct PPU *self) {
    self->controller = 0;
    self->mask = 0;
    self->status = 0;
    self->oam_address = 0;
    sekf->oam_data = 0;
    self->scroll = 0;
    self->address = 0;
    self->data = 0;
    self->oam_dma = 0;
    for (int i = 0; i < PPU_RAMSIZE; i++) {
        self->ram[i] = 0;
    }
    self->nmi_interrupt = 0;
}
