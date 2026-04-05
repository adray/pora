#include "poDWARFMachine.h"

#include <iostream>

using namespace po;

enum DW_LNS {
    DW_LNS_copy = 0x1,
    DW_LNS_advance_pc = 0x2,
    DW_LNS_advance_line = 0x3,
    DW_LNS_set_file = 0x4,
    DW_LNS_set_column = 0x5,
    DW_LNS_negate_stmt = 0x6,
    DW_LNS_set_basic_block = 0x7,
    DW_LNS_const_add_pc = 0x8,
    DW_LNS_fixed_advance_pc = 0x9,
    DW_LNS_set_prologue_end = 0xa,
    DW_LNS_set_epilogue_begin = 0xb,
    DW_LNS_set_isa = 0xc
};

enum DW_LINE {
    DW_LINE_end_sequence = 0x1,
    DW_LINE_set_address = 0x2,
    DW_LINE_set_discriminator = 0x4,
    DW_LINE_lo_user = 0x80,
    DW_LINE_hi_user = 0xff
};

void poDWARFMachine:: load(unsigned char* data, const int count) {
    _data = data;
    _count = count;
}

uint32_t poDWARFMachine:: readULEB128(int& pos) {
    uint32_t value = 0;
    int shift = 0;
    bool ok = true;
    while (ok) {
        const uint8_t byte = _data[pos++];
        value |= (uint32_t(byte & 0x7F) << shift);
        if (((byte >> 7) & 0x1) == 0) {
            ok = false;
        }
        shift += 7;
    }

    return value;
}

uint64_t poDWARFMachine::readAddress(int& pos) {
    const uint64_t value = 
        uint64_t(_data[pos]) |
        (uint64_t(_data[pos+1]) << 8) |
        (uint64_t(_data[pos+2]) << 16) |
        (uint64_t(_data[pos+3]) << 24) |
        (uint64_t(_data[pos+4]) << 32) |
        (uint64_t(_data[pos+5]) << 40) |
        (uint64_t(_data[pos+6]) << 48) |
        (uint64_t(_data[pos+7]) << 56);

    pos += 8;
    return value;
}

void poDWARFMachine:: writeRow(const int file, const int line, const int column, const uint64_t address) {
    _info.push_back(poDWARFLineInfo(file, line, column, address));

    //std::cout << "File: " << file << " Line: " << line << " Col: " << column << " Address: 0x" << std::hex << address << std::dec << std::endl;
}

void poDWARFMachine:: run(const bool defaultIsStmt, const int opcode_base, const int line_base, const int line_range, const int minimum_address_length, const int maximum_operation_per_instruction) {
    uint32_t file = 1;
    uint32_t line = 1;
    uint32_t column = 0;
    bool isStmt = defaultIsStmt;
    bool basicBlock = false;
    bool endSequence = false;
    bool prologueEnd = false;
    bool epilogueBegin = false;
    uint32_t isa = 0;
    uint32_t discriminator = 0;
    uint32_t op_index = 0;
    uint64_t address = 0;
    int pos = 0;

    while (pos < _count) {
        const int opcode = int(_data[pos++]);

        uint32_t code = 0;
        switch (opcode) {
            case DW_LNS_copy:
                writeRow(file, line, column, address);
                epilogueBegin = false;
                prologueEnd = false;
                basicBlock = false;
                discriminator = 0;
                break;
            case DW_LNS_advance_pc:
                code = readULEB128(pos);
                {
                    const int operation_advance = code;
                    const uint64_t newAddress = address + minimum_address_length * ((op_index + operation_advance) / maximum_operation_per_instruction);
                    const int newOpIndex = (op_index + operation_advance) % maximum_operation_per_instruction;
                    
                    address = newAddress;
                    op_index = newOpIndex;
                }
                break;
            case DW_LNS_fixed_advance_pc:
                op_index = 0;
                code = uint32_t(_data[pos]) | (uint32_t(_data[pos+1]) << 8);
                pos += 2;
                address += code;
                break;
            case DW_LNS_const_add_pc:
                code = 255;
                {
                    const int operation_advance = code;
                    const uint64_t newAddress = address + minimum_address_length * ((op_index + operation_advance) / maximum_operation_per_instruction);
                    const int newOpIndex = (op_index + operation_advance) % maximum_operation_per_instruction;
                    
                    address = newAddress;
                    op_index = newOpIndex;
                }
                break;
            case DW_LNS_advance_line:
                code = readULEB128(pos);
                line += code;
                break;
            case DW_LNS_set_file:
                file = readULEB128(pos);
                break;
            case DW_LNS_set_column:
                column = readULEB128(pos);
                break;
            case DW_LNS_negate_stmt:
                isStmt = !isStmt;
                break;
            case DW_LNS_set_basic_block:
                basicBlock = true;
                break;
            case DW_LNS_set_prologue_end:
                prologueEnd = true;
                break;
            case DW_LNS_set_epilogue_begin:
                epilogueBegin = true;
                break;
            case DW_LNS_set_isa:
                isa = readULEB128(pos);
                break;
            case 0: // extended opcode
                {
                    const uint32_t size = readULEB128(pos);
                    code = _data[pos++];
                    switch (code) {
                        case DW_LINE_end_sequence:
                            endSequence = true;
                            writeRow(file, line, column, address);
                            file = 1;
                            line = 1;
                            column = 0;
                            isStmt = defaultIsStmt;
                            basicBlock = false;
                            prologueEnd = false;
                            epilogueBegin = false;
                            isa = 0;
                            discriminator = 0;
                            op_index = 0;
                            address = 0;
                            endSequence = false;
                            break;
                        case DW_LINE_set_address:
                            op_index = 0;
                            address = readAddress(pos);
                            break;
                        case DW_LINE_set_discriminator:
                            discriminator = readULEB128(pos);
                            break;
                        default:
                            std::cout << "Unknown extended opcode: " << code << " pos: " << pos << " size: " << size << std::endl;
                            pos += size - 1;
                            break;
                    }
                }
                break;
            default: // special opcode
                {
                    const int adjusted_opcode = opcode - opcode_base;
                    const int operation_advance = adjusted_opcode / line_range;
                    const uint64_t newAddress = address + minimum_address_length * ((op_index + operation_advance) / maximum_operation_per_instruction);
                    const int newOpIndex = (op_index + operation_advance) % maximum_operation_per_instruction;
                    const int lineIncrement = line_base + (adjusted_opcode % line_range);

                    address = newAddress;
                    op_index = newOpIndex;
                    line += lineIncrement;
                }
 
                writeRow(file, line, column, address);

                basicBlock = false;
                prologueEnd = false;
                epilogueBegin = false;
                discriminator = 0;

                break;
        }
    }
}

