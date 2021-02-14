#include "config.hxx
#include "controller.hxx"
#ifdef CONTROLLER_DEBUG_OUTPUT
#include <iostream>
#endif

Controller::Controller() {
    this->state = 0;
    this->read_counter = 0;
    this->strobe = true;
}

bool Controller::get_button(Button button) {
    return this->state & static_cast<unsigned int>(button);
}

void Controller::set_button(Button button, bool on) {
    #ifdef CONTROLLER_DEBUG_OUTPUT
    std::cout << std::hex
              << "Setting button "
              << static_cast<unsigned int>(button)
              << " to state "
              << static_cast<unsigned int>(on)
              << std::endl;
    #endif
    if(on) {
        this->state |= static_cast<unsigned int>(button);
    }
    else {
        this->state &= ~static_cast<unsigned int>(button);
    }
}

void Controller::write_port_1(uint8_t value) {
    #ifdef CONTROLLER_DEBUG_OUTPUT
    std::cout << std::hex
              << "Writing to controller port 1 value "
              << static_cast<unsigned int>(value)
              << std::endl;
    #endif
    this->strobe = value & 0b1;
    if(this->strobe) {
        this->read_counter = 0;
    }
}

uint8_t Controller::read_port_1() {
    #ifdef CONTROLLER_DEBUG_OUTPUT
    std::cout << std::hex
              << "Reading controller port 1, strobe is "
              << static_cast<unsigned int>(this->strobe)
              << " read counter is "
              << static_cast<unsigned int>(this->read_counter)
              << std::endl;
    #endif
    if(this->strobe) {
        return this->get_button(Button::a);
    }
    else {
        this->read_counter++;
        switch(this->read_counter) {
            case 1:
                return this->get_button(Button::a);
            case 2:
                return this->get_button(Button::b);
            case 3:
                return this->get_button(Button::select);
            case 4:
                return this->get_button(Button::start);
            case 5:
                return this->get_button(Button::up);
            case 6:
                return this->get_button(Button::down);
            case 7:
                return this->get_button(Button::left);
            case 8:
                return this->get_button(Button::right);
            default:
                return 1;
        }
    }
}

uint8_t Controller::read_port_2() {
    return 0;
}