#ifndef CONTROLLER_HXX
#define CONTROLLER_HXX
#include <cstdint>

/* Buttons are read in order A, B, Select, Start, Up, Down, Left, Right. */

class Controller {
public:
    enum class Button {
        a       = 1 << 0,
        b       = 1 << 1,
        select  = 1 << 2,
        start   = 1 << 3,
        up      = 1 << 4,
        down    = 1 << 5,
        left    = 1 << 6,
        right   = 1 << 7
    };
    static const int PORT_1 = 0x4016;
    static const int PORT_2 = 0x4017;
private:
    uint8_t state;
    int read_counter;
    bool strobe;
    bool get_button(Button);
public:
    Controller();
    void reload();
    void set_button(Button, bool);
    void write_port_1(uint8_t);
    uint8_t read_port_1();
    uint8_t read_port_2();
};

#endif //CONTROLLER_HXX
