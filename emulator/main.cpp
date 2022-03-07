#include <iostream>
#include <fstream>
#include <vector>
#include <bitset>
#include <memory>
#include <unistd.h>
#include "memory.hpp"
#include "cpu.hpp"

using namespace std;

void load_program(shared_ptr<Memory>memory, string file_path) {
    ifstream ifs(file_path, ios::in | ios::binary);
    uint16_t buff;
    vector<uint16_t> program;
    while(!ifs.eof()) {
        ifs.read((char*)&buff, sizeof(uint16_t));
        program.push_back(buff);
    }
    for (int i = 0; i< program.size(); i++) {
        memory->memory[i] = program.at(i);
    }
}

void exit_with_help() {
    cerr << "[USAGE] emulator INPUT_FILE" << endl;
    exit(1);
}

int main(int argc, char *argv[]) {
    char *program_file = argv[1];

    if (argc < 2) {
        exit_with_help();
    }

    shared_ptr<CpuArch> arch(new CpuArch());
    shared_ptr<Memory> memory(new Memory());
    shared_ptr<Cpu> cpu(new Cpu(memory, arch));
    // プログラムをメモリに読み込む
    load_program(memory, string(program_file));

    // クロックを回す
    while(true) {
        if (cpu->clock()) {
            break;
        }
        // 人間にわかりやすいようにクロックは0.1秒sleepする
        usleep(100000);
    }

    // 0x64のアドレスは結果表示用とする
    cout << "RESULT is [" << memory->memory[0x64] << "]" << endl;

    return 0;
}
