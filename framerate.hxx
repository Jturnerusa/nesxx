#ifndef FRAMERATE_HXX
#define FRAMERATE_HXX
#include <chrono>
#include <thread>

class FrameRate {
private:
    int target_framerate;
    std::chrono::system_clock::time_point last_tick;
    std::chrono::duration<float, std::milli> get_delta();
public:
    FrameRate();
    void tick();
    int get_current_framerate();
    float get_frametime();
    void set_target_framerate(int);
    void sleep();
};

#endif //FRAMERATE_HXX
