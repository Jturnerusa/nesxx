#include <iostream>
#include <SDL2/SDL.h>
#include "config.hxx"
#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"
#include "rom.hxx"
#include "frame.hxx"
#include "color.hxx"


#if UNITTEST==1
int main() {
    test_bus();
    test_cpu();
}
#endif

#if UNITTEST==0
int main(int argc, char **argv) {
    Cpu cpu;
    Ppu ppu;
    Bus bus;
    auto rom = Rom(argv[1]);
    Frame frame;
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    ppu.connect_bus(&bus);
    ppu.connect_frame(&frame);
    cpu.reset();
    ppu.set_status_flag(StatusFlag::sprite_overflow, true);
    ppu.set_status_flag(StatusFlag::vblank, true);

}
#endif

