#ifndef FRAME_H
#define FRAME_H
#include <cstdint>
#include <vector>
#include "color.hxx"

class Frame {
public:
    Frame(int, int);
    int width, height;
    std::vector<uint32_t> buffer;
    void set_pixel(int, int, Color);
    uint32_t get_pixel(int, int);
    void clear(Color = Color::black);
};

#endif
