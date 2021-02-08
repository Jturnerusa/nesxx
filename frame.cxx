#include "frame.hxx"

namespace frame {

void Frame::set_pixel(int x, int y, uint32_t color) {
    int i = x + (y * this->WIDTH);
    this->buffer.at(i) = color;
}

uint32_t Frame::get_pixel(int x, int y) {
    int i = x + (y * this->WIDTH);
    return this->buffer.at(i);
}

void Frame::clear(uint32_t color) {
    this->buffer.fill(color);
}

int Frame::get_pitch() {
    return sizeof(uint32_t) * this->WIDTH;
}

}