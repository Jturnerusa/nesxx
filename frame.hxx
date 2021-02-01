#ifndef FRAME_H
#define FRAME_H
#include <cstdint>
#include <vector>

class Frame {
public:
    Frame(int, int);
    const int width, height;
    std::vector<uint32_t> buffer;
    void set_pixel(int, int, uint32_t);
    uint32_t get_pixel(int, int);
    void clear(uint32_t color = 0x00000000);
    int get_pitch();
};

#endif
