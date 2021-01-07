#include "frame.hxx"

void Frame::set_pixel(int x, int y, Color color) {
    int index = x * y;
    this->data.at(index) = static_cast<uint32_t>(color);
}

uint32_t Frame::get_pixel(int x, int y) {
    int index = x * y;
    return this->data.at(index);
}