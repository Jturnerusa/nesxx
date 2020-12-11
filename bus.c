#include <bus.h>

void init_bus(struct BUS *bus) {
    for (int i = 0; i < RAMSIZE; i++) {
        bus->ram[i] = 0;
    }
}

uint8_t *address read_ram(struct BUS *bus, uint16_t address) {
    
}

void *address read_ram(struct BUS *bus, uint16_t address, uint8_t data) {
    
}
