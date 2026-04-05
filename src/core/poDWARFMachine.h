#pragma once
#include <cstdint>
#include <vector>

namespace po {

    class poDWARFLineInfo {
        public:
            poDWARFLineInfo(const int file, const int line, const int col, const int address)
                :
                    _file(file),
                    _line(line),
                    _col(col),
                    _address(address)
            {
            }

            int file() const { return _file; }
            int line() const { return _line; }
            int col() const { return _col; }
            int address() const { return _address; }

        private:
            int _file;
            int _line;
            int _col;
            int _address;
    };

    class poDWARFMachine {
        public:
            void load(unsigned char* data, const int count);
            void run(const bool defaultIsStmt, const int opcode_base, const int line_base, const int line_range, const int minimum_address_length, const int maximum_operation_per_instruction);
        private:

            uint32_t readULEB128(int& pos);
            uint64_t readAddress(int& pos);
            void writeRow(const int file, const int line, const int column, const uint64_t address);
 
            unsigned char* _data;
            int _count;

            std::vector<poDWARFLineInfo> _info;
    };
}

