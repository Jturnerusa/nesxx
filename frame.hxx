#ifndef FRAME_H
#define FRAME_H
#include <cstdint>
#include <array>

const int SCREEN_WIDTH = 256;
const int SCREEN_HEIGHT = 240;

class Frame {
public:
    void set_pixel(int, int, uint8_t);
    uint8_t get_pixel(int, int);
private:
    std::array<uint8_t, SCREEN_WIDTH * SCREEN_HEIGHT> data;
};

#endif
