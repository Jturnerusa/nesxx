#include "config.hxx"

#ifdef NESTEST
#ifndef UNITTEST

#include "cpu.hxx"
#include "ppu.hxx"
#include "bus.hxx"
#include "rom.hxx"
int main(int argc, char **argv) {
    cpu::Cpu cpu;
    ppu::Ppu ppu;
    bus::Bus bus;
    rom::Rom rom;
    rom.load_from_file("/home/notroot/roms/nestest.nes");
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    ppu.connect_bus(&bus);
    cpu.prepare_for_nestest();
    while(true) {
        cpu.run_for(999);
    }
}

#endif
#endif

#ifdef UNITTEST
#ifndef NESTEST

#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"

int main() {
    run_bus_tests();
    run_ppu_tests();
}

#endif
#endif
#undef HEADLESS
#ifdef HEADLESS
#ifndef UNITTEST
#ifndef NESTEST

#include <iostream>
#include "cpu.hxx"
#include "bus.hxx"
#include "ppu.hxx"
#include "rom.hxx"
#include "frame.hxx"
#include "controller.hxx"

int main(int argc, char **argv) {
    Cpu cpu;
    Ppu ppu;
    Bus bus;
    Rom rom;
    Frame frame;
    Controller controller;
    rom.load_from_file(argv[1]);
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    bus.connect_controller(&controller);
    ppu.connect_bus(&bus);
    ppu.connect_frame(&frame);
    cpu.reset();
    ppu.reset();
    frame.clear();
    while(true) {
        cpu.run_for(1);
        ppu.render_scanline();
        std::cin.ignore();
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
#include "controller.hxx"

const int DISPLAY_WIDTH = 640;
const int DISPLAY_HEIGHT = 480;

int main(int argc, char **argv) {
    Cpu cpu;
    Ppu ppu;
    Bus bus;
    Rom rom;
    Frame frame;
    Controller controller;
    rom.load_from_file(argv[1]);
    cpu.connect_bus(&bus);
    bus.connect_ppu(&ppu);
    bus.connect_rom(&rom);
    bus.connect_controller(&controller);
    ppu.connect_bus(&bus);
    ppu.connect_frame(&frame);
    cpu.reset();
    ppu.reset();
    frame.clear();
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cout << "Failed to init SDL" << SDL_GetError();
        return 1;
    }
    SDL_Window   *window;
    SDL_Renderer *renderer;
    SDL_CreateWindowAndRenderer(DISPLAY_WIDTH, DISPLAY_HEIGHT, 0, &window, &renderer);
    SDL_SetWindowTitle(window, "Nesxx");
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(renderer, frame.WIDTH, frame.HEIGHT);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING,
                                             frame.WIDTH, frame.HEIGHT);
    SDL_Event event;
    const uint8_t *keys = SDL_GetKeyboardState(NULL);
    while (true) {
        while (SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    goto quit;
                case SDL_KEYDOWN:
                    controller.set_button(Controller::Button::up, keys[SDL_SCANCODE_W]);
                    controller.set_button(Controller::Button::left, keys[SDL_SCANCODE_A]);
                    controller.set_button(Controller::Button::down, keys[SDL_SCANCODE_S]);
                    controller.set_button(Controller::Button::right, keys[SDL_SCANCODE_D]);
                    controller.set_button(Controller::Button::a, keys[SDL_SCANCODE_RIGHT]);
                    controller.set_button(Controller::Button::b, keys[SDL_SCANCODE_DOWN]);
                    controller.set_button(Controller::Button::start, keys[SDL_SCANCODE_P]);
                    controller.set_button(Controller::Button::select, keys[SDL_SCANCODE_O]);
                    break;
                case SDL_KEYUP:
                    controller.set_button(Controller::Button::up, keys[SDL_SCANCODE_W]);
                    controller.set_button(Controller::Button::left, keys[SDL_SCANCODE_A]);
                    controller.set_button(Controller::Button::down, keys[SDL_SCANCODE_S]);
                    controller.set_button(Controller::Button::right, keys[SDL_SCANCODE_D]);
                    controller.set_button(Controller::Button::a, keys[SDL_SCANCODE_RIGHT]);
                    controller.set_button(Controller::Button::b, keys[SDL_SCANCODE_DOWN]);
                    controller.set_button(Controller::Button::start, keys[SDL_SCANCODE_P]);
                    controller.set_button(Controller::Button::select, keys[SDL_SCANCODE_O]);
                    break;
            }
        }
        cpu.run_for(115);
        ppu.render_scanline();
        if (ppu.get_scanline() == 240) {
            SDL_UpdateTexture(texture, NULL, frame.buffer.data(), frame.get_pitch());
            SDL_RenderCopy(renderer, texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }
    }
    quit:
    SDL_Quit();
    return 0;
}

#endif
#endif
#endif
