#ifndef EMULATOR_CPU_HPP
#define EMULATOR_CPU_HPP

#include <iostream>
#include <vector>
#include <bitset>
#include <map>
#include <memory>
#include <unistd.h>
#include <iomanip>
#include "alu.hpp"
#include "psw.hpp"
#include "memory.hpp"
#include "arch.hpp"

using namespace std;

enum class CpuStatus {
    FETCH_INST_0,  // 命令フェッチ0
    FETCH_INST_1,  // 命令フェッチ1
    FETCH_OPERAND_0,  // オペランドフェッチ0
    FETCH_OPERAND_1,  // オペランドフェッチ1
    EXEC_INST,  // 演算実行
    WRITE_BACK,  // WriteBack (レジスタ、メモリへの下記戻しなど)
};

class Cpu {
private:

    void print_info() {
        int index = 0;
        cout << "\033[2J" <<"\033[0;0H";  // clear screen
        cout << endl << "----------CLOCK------------" << endl;
        cout << " " << "CLOCK [" << dec << clock_counter << "]" << endl;
        cout << endl << "-------INSTRUCTION---------" << endl;
        if (current_program != nullptr) {
            cout << " " << "IR [ " << current_program->inst.mnemonic << " ";
            if (current_program->inst.operand_type == OperandType::SINGLE_OPERAND ||
                current_program->inst.operand_type == OperandType::DOUBLE_OPERAND) {
                cout << bitset<3>(current_program->first_operand) << " ";
                if (current_program->inst.operand_type == OperandType::DOUBLE_OPERAND) {
                    cout << bitset<8>(current_program->second_operand) << " ";
                }
            }
            cout << "]" << endl;
        }
        cout << endl << "------STATUS COUNTER--------" << endl;
        for (int i = 0; i < statuses.size(); i++) {
            cout << " " << statuses[static_cast<CpuStatus>(i)] << " ";
            if (current_status == static_cast<CpuStatus>(i)) {
                cout << " <";
            }
            cout << endl;
        }
        cout << endl << "---------REGISTER-----------" << endl;
        for(uint16_t reg: registers) {
            stringstream hex_str;
            cout << " " <<  "R" << index << " [" << bitset<16>(reg) << "] (0x" << hex << reg << ") ";
            if (index == arch->PC_REG_NUMBER) {
                cout << " (PC) ";
            }
            if (index == arch->SP_REG_NUMBER) {
                cout << " (SP) ";
            }
            if (index == arch->PSW_REG_NUMBER) {
                cout << "N = "<< psw->get_negative_flag() << " ";
                cout << "Z = "<< psw->get_zero_flag() << " ";
                cout << " (PSW) ";
            }
            cout << endl;
            index++;
        }
        cout << endl << "----------MEMORY------------" << endl;
        int min_memory_index = (registers[arch->PC_REG_NUMBER] - 2) >= 0 ? registers[arch->PC_REG_NUMBER] - 2 : 0;
        for (int i = min_memory_index; i < (min_memory_index + 5); i++) {
            cout << " " << "[0x" << hex << i << "] [" << bitset<16>(memory->memory[i]) << "] ";
            if (i == registers[arch->PC_REG_NUMBER]) {
                cout << "(PC)";
            }
            cout << endl;
        }
        cout << endl;
        cout << " " << "[0x64] [" << bitset<16>(memory->memory[0x64]) << "] (" << dec << memory->memory[0x64] << ") " << endl;
        cout << "--------------------------" << endl;
    }


public:
    shared_ptr<Alu> alu;
    shared_ptr<Psw> psw;
    shared_ptr<Memory> memory;
    shared_ptr<CpuArch> arch;

    uint16_t mar = 0;
    uint16_t mdr = 0;

    map<CpuStatus, string> statuses;

    vector<uint16_t> registers;
    uint16_t reg_b = 0;
    uint16_t ir = 0;

    uint16_t s_bus = 0;
    uint16_t a_bus = 0;
    uint16_t b_bus = 0;
    int clock_counter = 1;
    CpuStatus current_status = CpuStatus::FETCH_INST_0;
    shared_ptr<Program> current_program;

    Cpu() {

    }
    Cpu(shared_ptr<Memory> memory, shared_ptr<CpuArch> arch) {
        clock_counter = 1;

        // デバッグしやすいように文字列にしておく
        statuses[CpuStatus::FETCH_INST_0] = "FETCH_INST_0";
        statuses[CpuStatus::FETCH_INST_1] = "FETCH_INST_1";
        statuses[CpuStatus::FETCH_OPERAND_0] = "FETCH_OPERAND_0";
        statuses[CpuStatus::FETCH_OPERAND_1] = "FETCH_OPERAND_1";
        statuses[CpuStatus::EXEC_INST] = "EXEC_INST";
        statuses[CpuStatus::WRITE_BACK] = "WRITE_BACK";

        this->memory = memory;
        this->arch = arch;
        this->registers = vector<uint16_t>();
        for (auto r: arch->registers) {
            this->registers.push_back(0);
        }
        this->psw = shared_ptr<Psw>(new Psw(&this->registers[arch->PSW_REG_NUMBER]));
        this->alu = shared_ptr<Alu>(new Alu(this->psw));
    }

    // クロック時の処理
    bool clock() {
        bool is_hlt = false;
        switch (current_status) {
            case CpuStatus::FETCH_INST_0:
                a_bus = registers[arch->PC_REG_NUMBER];
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::FETCH_INST_1;
                break;
            case CpuStatus::FETCH_INST_1:
                mar = s_bus;
                memory->mode = MemoryMode::READ;
                memory->access(&mar, &mdr);
                alu->mode = AluMode::INC;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::FETCH_OPERAND_0;
                break;
            case CpuStatus::FETCH_OPERAND_0:
                ir = mdr;
                current_program = Program::decode(ir, arch);
                registers[arch->PC_REG_NUMBER] = s_bus;
                if (current_program->inst.type == InstructionType::MOV
                    || current_program->inst.type == InstructionType::ADD
                    || current_program->inst.type == InstructionType::SUB
                    || current_program->inst.type == InstructionType::AND
                    || current_program->inst.type == InstructionType::OR
                    || current_program->inst.type == InstructionType::CMP) {
                    a_bus = registers[current_program->second_operand >> 5];
                    current_status = CpuStatus::EXEC_INST;
                } else if (current_program->inst.type == InstructionType::SL || current_program->inst.type == InstructionType::SR) {
                    a_bus = registers[current_program->first_operand];
                    current_status = CpuStatus::EXEC_INST;
                }else if (current_program->inst.type == InstructionType::LD || current_program->inst.type == InstructionType::ST) {
                        a_bus = current_program->second_operand;
                        current_status = CpuStatus::FETCH_OPERAND_1;
                } else {
                    current_status = CpuStatus::EXEC_INST;
                }
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);

                break;
            case CpuStatus::FETCH_OPERAND_1:
                mar = s_bus;
                if (current_program->inst.type == InstructionType::LD) {
                    memory->mode = MemoryMode::READ;
                    memory->access(&mar, &mdr);
                }
                alu->mode = AluMode::NOP;
                s_bus = alu->calc(a_bus, b_bus);
                current_status = CpuStatus::EXEC_INST;
                break;
            case CpuStatus::EXEC_INST:
                switch (current_program->inst.type) {
                    case InstructionType::HLT:
                        is_hlt = true;
                        break;
                    case InstructionType::ADD:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::ADD;
                        break;
                    case InstructionType::SUB:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::SUB;
                        break;
                    case InstructionType::AND:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::AND;
                        break;
                    case InstructionType::OR:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::OR;
                        break;
                    case InstructionType::SL:
                        alu->mode = AluMode::SHIFT_L;
                        break;
                    case InstructionType::SR:
                        alu->mode = AluMode::SHIFT_R;
                        break;
                    case InstructionType::LDL:
                        a_bus = current_program->second_operand;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::LDH:
                        a_bus = current_program->second_operand << 8;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::CMP:
                        reg_b = s_bus;
                        b_bus = reg_b;
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::CMP;
                        break;
                    case InstructionType::JE:
                        a_bus = current_program->second_operand;
                        current_program->first_operand = arch->PC_REG_NUMBER;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::JMP:
                        a_bus = current_program->second_operand;
                        current_program->first_operand = arch->PC_REG_NUMBER;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::LD:
                        a_bus = mdr;
                        alu->mode = AluMode::NOP;
                        break;
                    case InstructionType::ST:
                        a_bus = registers[current_program->first_operand];
                        alu->mode = AluMode::NOP;
                        break;
                }
                s_bus = alu->calc(a_bus, b_bus);

                current_status = CpuStatus::WRITE_BACK;
                break;
            case CpuStatus::WRITE_BACK:
                if (current_program->inst.type == InstructionType::ST) {
                    mdr = s_bus;
                    memory->mode = MemoryMode::WRITE;
                    memory->access(&mar, &mdr);
                } else if (current_program->inst.type == InstructionType::LDL || current_program->inst.type == InstructionType::LDH) {
                    // 上位、下位にそれぞれbitを別命令として入れるのでOR
                    registers[current_program->first_operand] |= s_bus;
                } else if(current_program->inst.type == InstructionType::JE) {
                    if (psw->get_zero_flag()) {
                        registers[current_program->first_operand] = s_bus;
                    }
                } else if (current_program->inst.type == InstructionType::CMP) {
                    // pass
                } else {
                    registers[current_program->first_operand] = s_bus;
                }
                current_status = CpuStatus::FETCH_INST_0;
                break;
        }
        print_info();
        clock_counter++;
        return is_hlt;
    }

};


#endif //EMULATOR_CPU_HPP
