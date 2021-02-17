#include "framerate.hxx"

FrameRate::FrameRate() {
    this->last_tick = std::chrono::system_clock::now();
}

std::chrono::duration<float, std::milli> FrameRate::get_delta() {
    std::chrono::duration<float, std::milli> delta = std::chrono::system_clock::now() - this->last_tick;
    return delta;
}

void FrameRate::tick() {
    this->last_tick = std::chrono::system_clock::now();
}

void FrameRate::set_target_framerate(int framerate) {
    this->target_framerate = framerate;
}

int FrameRate::get_current_framerate() {
    auto delta = this->get_delta();
    return 1000 / delta.count();
}

float FrameRate::get_frametime() {
    auto delta = this->get_delta();
    return delta.count();
}

void FrameRate::sleep() {
    std::chrono::time_point window = this->last_tick + std::chrono::milliseconds(1000 / this->target_framerate);
    std::chrono::time_point now = std::chrono::system_clock::now();
    std::chrono::duration<float, std::milli> delta = window - now;
    if(delta.count() > 0) {
        std::this_thread::sleep_for(delta);
    }
}