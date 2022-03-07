#ifndef EMULATOR_MEMORY_HPP
#define EMULATOR_MEMORY_HPP

#include <memory>

using namespace std;

const int MEMORY_SIZE = 256;

enum class MemoryMode {
    READ,
    WRITE
};

class Memory {
public:
    MemoryMode mode;
    uint16_t mem_bus_addr;
    uint16_t mem_bus_data;

    uint16_t memory[MEMORY_SIZE] = {0};
    Memory() {
        this->mem_bus_addr = 0;
        this->mem_bus_data = 0;
        this->mode = MemoryMode::READ;
    }

    void access(uint16_t* mar, uint16_t* mdr) {
        if (this->mode == MemoryMode::READ) {
            mem_bus_addr = *mar;
            mem_bus_data = this->memory[mem_bus_addr];
            *mdr = mem_bus_data;
        } else {
            mem_bus_data = *mdr;
            mem_bus_addr = *mar;
            this->memory[mem_bus_addr] = mem_bus_data;
        }
    }

};

#endif //EMULATOR_MEMORY_HPP
