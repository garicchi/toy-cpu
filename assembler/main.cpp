#include <iostream>
#include <fstream>
#include <vector>
#include <memory>
#include <map>
#include <bitset>
#include "arch.hpp"

using namespace std;

enum class TokenType {
    RESERVED,  // 予約語(命令のニーモニック、レジスタ名)
    HEX,  // 16進数の数字
    IDENT,  // 識別子(ラベル文字)
    EOL,  // 改行
    COMMA, // コンマ
    COLON  // コロン
};

struct Token {
    string str;
    TokenType type;
};

shared_ptr<vector<uint16_t>> generate(shared_ptr<vector<Program>> programs) {
    // プログラムの構造をバイナリコードに変換する
    shared_ptr<vector<uint16_t>> output_code(new vector<uint16_t>());
    uint16_t code;
    for (auto program: *programs) {
        code = Program::assemble(program);
        output_code->push_back(code);
        string debug_asm = "[" + to_string(output_code->size()) + "] ";
        debug_asm += program.inst.mnemonic + " ";
        if (program.inst.operand_type == OperandType::SINGLE_OPERAND) {
            debug_asm += to_string(program.first_operand);
        } else if (program.inst.operand_type == OperandType::DOUBLE_OPERAND) {
            debug_asm += to_string(program.first_operand) + ", " + to_string(program.second_operand);
        }
        cout << debug_asm << "\t" << bitset<16>(code) << endl;
    }
    return output_code;
}

uint16_t resolve_operand(shared_ptr<CpuArch> arch, Token operand_token, shared_ptr<map<string, int>> label_table) {
    // オペランドを16bitコードに変換する
    if (operand_token.type == TokenType::HEX) {
        return stoi(operand_token.str, 0, 16);
    }
    if (label_table->find(operand_token.str) != label_table->end()) {
        return label_table->at(operand_token.str);
    }
    auto reg = arch->get_register_by_name(operand_token.str);
    if (reg) {
        return reg->code;
    }
    cerr << "invalid operand " << operand_token.str << endl;
    exit(1);
    return NULL;
}

shared_ptr<vector<Program>> parse(shared_ptr<CpuArch> arch, shared_ptr<vector<Token>> tokens) {
    shared_ptr<map<string, int>> label_table(new map<string, int>());  // ラベル文字列と行番号のマップ
    // トークンをシークしてラベル名と行数のマップを構築しておく
    //   ラベルは事前に行番号に解決しておく必要がある
    int code_index = 0;
    for (int current_pos = 0; current_pos < tokens->size();) {
        Token *first_token = &tokens->at(current_pos);
        Token *second_token = &tokens->at(current_pos + 1);
        // 空行ならスキップ
        if (first_token->type == TokenType::EOL) {
            current_pos++;
            continue;
        }
        // ラベルならラベルマップに入れる
        if (first_token->type == TokenType::IDENT && second_token->type == TokenType::COLON) {
            label_table->insert(make_pair(first_token->str, code_index));
            current_pos += 3;
            continue;
        }
        // 命令ならコード行数を加算してスキップ
        if (second_token->type == TokenType::EOL && first_token->type != TokenType::COLON) {
            code_index++;
            current_pos += 2;
            continue;
        }
        current_pos++;
    }

    // トークンをシークしてプログラムの構造に変換する
    shared_ptr<vector<Program>> programs(new vector<Program>());
    for (int current_pos = 0; current_pos < tokens->size();) {
        Token *first_token = &tokens->at(current_pos++);
        if (first_token->type != TokenType::IDENT && first_token->type != TokenType::EOL) {
            // 識別子(ラベル名)か空行でないなら命令としてパースする
            // 命令取得
            auto inst = arch->get_inst_by_mnemonic(first_token->str);
            if (!inst) {
                cout << "invalid inst [" << first_token->str << "]" << endl;
            }
            // オペランド取得
            Token *first_operand_token = NULL;
            Token *second_operand_token = NULL;
            if (tokens->at(current_pos).type != TokenType::EOL && tokens->at(current_pos).type != TokenType::COMMA) {
                first_operand_token = &tokens->at(current_pos++);
            }
            if (tokens->at(current_pos).type == TokenType::COMMA) {
                tokens->at(current_pos++); // comma
                second_operand_token = &tokens->at(current_pos++);
            }
            uint16_t first_operand = 0;
            uint16_t second_operand = 0;
            // 命令の定義からオペランドの数を取得して解析
            if (inst->operand_type == OperandType::SINGLE_OPERAND) {
                first_operand = resolve_operand(arch, *first_operand_token, label_table);
                if (first_operand_token->type == TokenType::RESERVED) {
                    first_operand <<= 8;
                }
            } else if (inst->operand_type == OperandType::DOUBLE_OPERAND) {
                first_operand = resolve_operand(arch, *first_operand_token, label_table);
                second_operand = resolve_operand(arch, *second_operand_token, label_table);
                // レジスターは3bitなので第2オペランドがレジスターの場合は5bit左シフト
                if (second_operand_token->type == TokenType::RESERVED) {
                    second_operand <<= 5;
                }

            }
            programs->push_back(Program(inst.value(), first_operand, second_operand));
        } else {
            // 識別子ならラベル名なのでトークンを読み取りスキップする
            if (first_token->type == TokenType::IDENT) {
                current_pos++;
            }
        }
    }
    return programs;
}

shared_ptr<vector<Token>> tokenize(shared_ptr<CpuArch> arch, shared_ptr<string> code_str) {
    shared_ptr<vector<Token>> tokens(new vector<Token>());
    string current_token_str;
    bool is_among_comment = false;
    // 1文字ずつシークしてトークンを追加していく
    for (int current_pos = 0; current_pos < code_str->length(); current_pos++) {
        // 現在の文字
        char current_char = code_str->at(current_pos);
        // 次の文字があるなら取得しておく
        char next_char = NULL;
        if ((current_pos + 1) < code_str->length()) {
            next_char = code_str->at(current_pos + 1);
        }
        // コメント中かどうかを判別
        if (current_char == ';') {
            is_among_comment = true;
        }
        if (is_among_comment && current_char == '\n') {
            is_among_comment = false;
        }
        if (is_among_comment) {
            continue;
        }
        // 無視してもいい文字列以外なら現在のトークン文字列に追加する
        if (current_char != ' ' && current_char != '\t') {
            current_token_str.push_back(current_char);
        }

        // 現在の文字列が1文字で完結するトークンならトークンに追加する
        if (current_char == ',' || current_char == ':' || current_char == '\n') {
            TokenType token_type = TokenType::COMMA;
            if (current_char == ',') {
                token_type = TokenType::COMMA;
            }
            if (current_char == ':') {
                token_type = TokenType::COLON;
            }
            if (current_char == '\n') {
                token_type = TokenType::EOL;
            }
            Token token;
            token.str = current_token_str;
            token.type = token_type;
            tokens->push_back(token);
            current_token_str.clear();
        } else if (next_char == ' ' || next_char == '\t' || next_char == ',' || next_char == '\n' || next_char == ':') {
            // 次の文字がトークンの区切り文字なら現在までのトークン文字列をトークンとして追加する
            TokenType token_type = TokenType::IDENT;
            if (current_token_str.substr(0, 2) == "0x") {
                token_type = TokenType::HEX;
            }

            auto inst = arch->get_inst_by_mnemonic(current_token_str);
            if (inst) {
                token_type = TokenType::RESERVED;
            }
            auto reg = arch->get_register_by_name(current_token_str);
            if (reg) {
                token_type = TokenType::RESERVED;
            }

            Token token;
            token.str = current_token_str;
            token.type = token_type;
            tokens->push_back(token);
            current_token_str.clear();
        }

    }
    return tokens;
}

shared_ptr<string> read_file(char* file_path) {
    shared_ptr<string> text(new string());
    ifstream ifs(file_path);
    string line;
    while(getline(ifs, line)) {
        text->append(line + '\n');
    }
    ifs.close();
    return text;
}

void write_code(char* file_path, shared_ptr<vector<uint16_t>> code) {
    // バイナリを書き込む
    //   書き込むバイト数はu16_t(2byte)のコード数
    ofstream ofs(file_path, ios::binary);
    ofs.write((char*)&code->at(0), code->size() * sizeof(uint16_t));
    ofs.close();
}

void exit_with_help() {
    cerr << "[USAGE] assembler INPUT_FILE OUTPUT_FILE" << endl;
    exit(1);
}

int main(int argc, char *argv[]) {
    char* input_file = argv[1];
    char *output_file = argv[2];
    if (argc < 3) {
        exit_with_help();
    }

    shared_ptr<CpuArch> arch(new CpuArch());
    shared_ptr<string> program_text = read_file(input_file);

    // 文字列をシークしてトークン列化していく
    auto tokens = tokenize(arch, program_text);
    // トークン列をプログラムの構造にパースする
    auto programs = parse(arch, tokens);
    // トークン列からコード(bianry)を生成する
    auto code = generate(programs);

    write_code(output_file, code);

    cout << "output binary in " << output_file << endl;

    return 0;
}
