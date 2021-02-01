#include "frame.hxx"

Frame::Frame(int width, int height):width(width),height(height) {
    this->buffer.resize(this->width * this->height);
}

void Frame::set_pixel(int x, int y, uint32_t color) {
    int i = x + (y * this->width);
    this->buffer.at(i) = color;
}

uint32_t Frame::get_pixel(int x, int y) {
    int i = x + (y * this->width);
    return this->buffer.at(i);
}

void Frame::clear(uint32_t color) {
    std::fill(this->buffer.begin(), this->buffer.end(), color);
}

int Frame::get_pitch() {
    return sizeof(uint32_t) * this->width;
}