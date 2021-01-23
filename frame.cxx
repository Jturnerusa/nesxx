#include "frame.hxx"

Frame::Frame(int width, int height) {
    this->width = width;
    this->height = height;
    this->buffer.resize(this->width * this->height);
}

void Frame::set_pixel(int x, int y, Color color) {
    int i = x + (y * this->height);
    this->buffer.at(i) = static_cast<uint32_t>(color);
}

uint32_t Frame::get_pixel(int x, int y) {
    int i = x + (y * this->height);
    return this->buffer.at(i);
}

void Frame::clear(Color color) {
    std::fill(this->buffer.begin(), this->buffer.end(), static_cast<uint32_t>(color));
}