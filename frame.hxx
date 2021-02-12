#ifndef FRAME_H
#define FRAME_H
#include <cstdint>
#include <array>

class Frame {
public:
    static const int WIDTH = 256;
    static const int HEIGHT = 240;
    std::array<uint32_t, WIDTH * HEIGHT> buffer;
    void set_pixel(int, int, uint32_t);
    uint32_t get_pixel(int, int);
    void clear(uint32_t color = 0x00000000);
    int get_pitch();
};

#endif