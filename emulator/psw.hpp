#ifndef EMULATOR_PSW_HPP
#define EMULATOR_PSW_HPP

#include <memory>

using namespace std;

class Psw {
private:
    uint16_t* reg_psw;
public:
    Psw() {

    }
    Psw(uint16_t* reg_psw) {
        this->reg_psw = reg_psw;
    }

    void set_negative_flag(bool is_on) {
        uint16_t mask = 0b1000000000000000;
        if (is_on) {
            *(this->reg_psw) |= mask;
        } else {
            *this->reg_psw &= ~mask;
        }
    }

    void set_zero_flag(bool is_on) {
        uint16_t mask = 0b0100000000000000;
        if (is_on) {
            *(this->reg_psw) |= mask;
        } else {
            *(this->reg_psw) &= ~mask;
        }
    }

    uint16_t get_negative_flag() {
        uint16_t mask = 0b1000000000000000;
        return (*(this->reg_psw) >> 15) & 0x1;
    }

    uint16_t get_zero_flag() {
        uint16_t mask = 0b0100000000000000;
        return (*(this->reg_psw) >> 14) & 0x1;
    }
};
#endif //EMULATOR_PSW_HPP
