#ifndef EMULATOR_ALU_HPP
#define EMULATOR_ALU_HPP

#include <memory>
#include "psw.hpp"

using namespace std;

enum class AluMode {
    ADD,
    SUB,
    CMP,
    AND,
    OR,
    XOR,
    NOT,
    SHIFT_L,
    SHIFT_R,
    INC,
    DEC,
    NOP
};


class Alu {
public:
    AluMode mode;
    shared_ptr<Psw> psw;
    Alu() {

    }
    Alu(shared_ptr<Psw> psw) {
        this->psw = psw;
        this->mode = AluMode::NOP;
    }

    uint16_t calc(uint16_t a_bus, uint16_t b_bus) {
        uint16_t result = 0;
        switch (this->mode) {
            case AluMode::ADD:
                result = a_bus + b_bus;
                break;
            case AluMode::SUB:
                result = a_bus - b_bus;
                break;
            case AluMode::AND:
                result = a_bus & b_bus;
                break;
            case AluMode::OR:
                result = a_bus | b_bus;
                break;
            case AluMode::SHIFT_L:
                result = a_bus << 1;
                break;
            case AluMode::SHIFT_R:
                result = a_bus >> 1;
                break;
            case AluMode::CMP:
                result = a_bus - b_bus;
                if (result < 0) {
                    this->psw->set_negative_flag(1);
                } else if (result > 0) {
                    this->psw->set_negative_flag(0);
                }
                if (result == 0) {
                    this->psw->set_zero_flag(1);
                } else {
                    this->psw->set_zero_flag(0);
                }
                break;
            case AluMode::INC:
                result = a_bus + 1;
                break;
            case AluMode::DEC:
                result = a_bus - 1;
                break;
            case AluMode::NOP:
                result = a_bus;
                break;
        }
        return result;
    }

};

#endif //EMULATOR_ALU_HPP
