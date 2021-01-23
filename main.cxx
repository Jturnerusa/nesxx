#include "config.hxx"

#ifdef NESTEST
#ifndef UNITTEST

#include "cpu.hxx"
#include "ppu.hxx"
#include "bus.hxx"
#include "rom.hxx"
int main(int argc, char **argv) {
    Cpu cpu;
    Ppu ppu;
    Bus bus;
    auto rom = Rom("/home/notroot/roms/nestest.nes");
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    ppu.connect_bus(&bus);
    cpu.prepare_for_nestest();
    while(true) {
        cpu.run_instruction();
    }
}

#endif
#endif
#endif

#ifdef HEADLESS
#ifndef UNITTEST
#ifndef NESTEST

#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"
#include "rom.hxx"
#include "frame.hxx"

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
    ppu.reset();
    while(true) {
        cpu.run_instruction();
        ppu.tick(cpu.get_opcode_cycles());
    }
    return 0;
}

#endif
#endif
#endif


#ifndef HEADLESS
#ifndef UNITTEST
#ifndef NESTEST

#include <iostream>
#include <SDL2/SDL.h>
#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"
#include "rom.hxx"
#include "frame.hxx"

int main(int argc, char **argv) {
    Cpu   cpu;
    Ppu   ppu;
    Bus   bus;
    auto  rom = Rom(argv[1]);
    Frame frame(SCREEN_WIDTH, SCREEN_HEIGHT);
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    ppu.connect_bus(&bus);
    ppu.connect_frame(&frame);
    cpu.reset();
    ppu.reset();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Failed to init SDL" << SDL_GetError();
        return 1;
    }
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(SCREEN_WIDTH, SCREEN_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "Nesxx");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                             SCREEN_WIDTH, SCREEN_HEIGHT);
    SDL_Event   event;
    while (true) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                SDL_Quit();
                return 0;
            }
        }
        cpu.run_instruction();
        ppu.tick(cpu.get_opcode_cycles());
        if (ppu.get_scanline() == 240) {
            SDL_RenderClear(renderer);
            SDL_UpdateTexture(texture, NULL, frame.buffer.data(), sizeof(uint32_t) * SCREEN_HEIGHT);
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
}

#endif
#endif
#endif