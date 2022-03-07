#include <memory>

#ifndef CPU_BASIC_ARCH_HPP
#define CPU_BASIC_ARCH_HPP

// memory layout of instruction
// instruction using double register
// 15  14 13 12 11   10 9 8   7 6 5   4 3 2 1 0
// -   inst           source   dest    ----

// instruction using register and 8bit data
// 15  14 13 12 11   10 9 8   7 6 5 4 3 2 1 0
// -   inst          source   data

// instruction using single register
// 15  14 13 12 11   10 9 8   7 6 5 4 3 2 1 0
// -   inst           reg     -----

// instruction using single data
// 15  14 13 12 11   10 9 8 7 6 5 4 3   2 1 0
// -   inst           data              -----

using namespace std;

enum class InstructionType {
    MOV,
    ADD,
    SUB,
    AND,
    OR,
    SL,
    SR,
    LDL,
    LDH,
    CMP,
    JE,
    JMP,
    LD,
    ST,
    HLT
};

enum class OperandType {
    SINGLE_OPERAND,
    DOUBLE_OPERAND,
    NO_OPERAND
};

// 命令の定義
class Instruction {
public:
    InstructionType type;
    OperandType operand_type;
    uint16_t opcode;
    string mnemonic;
    Instruction() {

    }
    Instruction(InstructionType type, string mnemonic, uint16_t opcode, OperandType operand_type) {
        this->type = type;
        this->operand_type = operand_type;
        this->mnemonic = mnemonic;
        this->opcode = opcode;
    }
};

// レジスタの定義
class Register {
public:
    string name;
    uint16_t code;  // 3bitしかつかわないけど
    Register() {

    }
    Register(string name, uint16_t code) {
        this->name = name;
        this->code = code;
    }
};

// CPUのアーキテクチャの構造
class CpuArch {
public:
    vector<Instruction> instructions = {
            Instruction(InstructionType::MOV, "mov", 0b0000, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::ADD, "add", 0b0001, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::SUB, "sub", 0b0010, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::AND, "and", 0b0011, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::OR, "or", 0b0100, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::SL, "sl", 0b0101, OperandType::SINGLE_OPERAND),
            Instruction(InstructionType::SR, "sr", 0b0110, OperandType::SINGLE_OPERAND),
            Instruction(InstructionType::LDL, "ldl", 0b1000, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::LDH, "ldh", 0b1001, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::CMP, "cmp", 0b1010, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::JE, "je", 0b1011, OperandType::SINGLE_OPERAND),
            Instruction(InstructionType::JMP, "jmp", 0b1100, OperandType::SINGLE_OPERAND),
            Instruction(InstructionType::LD, "ld", 0b1101, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::ST, "st", 0b1110, OperandType::DOUBLE_OPERAND),
            Instruction(InstructionType::HLT, "hlt", 0b1111, OperandType::NO_OPERAND)
    };

    vector<Register> registers = {
            Register("r0", 0b000),
            Register("r1", 0b001),
            Register("r2", 0b010),
            Register("r3", 0b011),
            Register("r4", 0b100),
            Register("r5", 0b101), // PSW
            Register("r6", 0b110), // Stack Pointer
            Register("r7", 0b111), // Program Counter
    };

    const int PSW_REG_NUMBER = 5;
    const int SP_REG_NUMBER = 6;
    const int PC_REG_NUMBER = 7;

    CpuArch() {

    }

    optional<Instruction> get_inst_by_mnemonic(string mnemonic) {
        int find_index = -1;
        for (int i = 0; i < this->instructions.size(); i++) {
            if (this->instructions[i].mnemonic == mnemonic) {
                find_index = i;
                break;
            }
        }
        if (find_index >= 0) {
            return this->instructions[find_index];
        } else {
            return nullopt;
        }
    }

    optional<Instruction> get_inst_by_opcode(uint16_t opcode) {
        int find_index = -1;
        for (int i = 0; i < this->instructions.size(); i++) {
            if (this->instructions[i].opcode == opcode) {
                find_index = i;
                break;
            }
        }
        if (find_index >= 0) {
            return this->instructions[find_index];
        } else {
            return nullopt;
        }
    }

    optional<Register> get_register_by_name(string name) {
        int find_index = -1;
        for (int i = 0; i < this->registers.size(); i++) {
            if (this->registers[i].name == name) {
                find_index = i;
                break;
            }
        }
        if (find_index >= 0) {
            return this->registers[find_index];
        } else {
            return nullopt;
        }
    }
};

// 命令とオペランドの構造
class Program {
public:
    Instruction inst;
    uint16_t first_operand;
    uint16_t second_operand;
    Program() {

    }
    Program(Instruction inst, uint16_t first_operand, uint16_t second_operand) {
        this->inst = inst;
        this->first_operand = first_operand;
        this->second_operand = second_operand;
    }

    static uint16_t assemble(Program p) {
        uint16_t code = 0;

        code |= p.inst.opcode << 11;
        if (p.inst.operand_type == OperandType::SINGLE_OPERAND) {
            code |= p.first_operand;
        } else if (p.inst.operand_type == OperandType::DOUBLE_OPERAND) {
            code |= p.first_operand << 8;
            code |= p.second_operand;
        }
        return code;
    }

    static shared_ptr<Program> decode(uint16_t code, shared_ptr<CpuArch> arch) {
        uint16_t mask_opcode = 0b0111100000000000;
        uint16_t mask_first_operand = 0b0000011100000000;
        uint16_t mask_second_operand = 0b0000000011111111;

        uint16_t opcode = (code & mask_opcode) >> 11;
        auto inst = arch->get_inst_by_opcode(opcode);

        if (!inst) {
            cerr << "invalid opcode " << opcode << endl;
            exit(1);
        }

        uint16_t first_operand = (code & mask_first_operand) >> 8;
        uint16_t second_operand = (code & mask_second_operand);

        return shared_ptr<Program>(new Program(inst.value(), first_operand, second_operand));
    }

};



#endif //CPU_BASIC_ARCH_HPP
