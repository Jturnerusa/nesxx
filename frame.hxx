#ifndef FRAME_H
#define FRAME_H
#include <cstdint>
#include <array>
#include "color.hxx"

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;

class Frame {
public:
    void set_pixel(int, int, Color);
    uint32_t get_pixel(int, int);
    void clear();
private:
    std::array<uint32_t, SCREEN_WIDTH * SCREEN_HEIGHT + 1> data;
};

#endif
