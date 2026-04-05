#pragma once
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <string>

namespace po {
    class poDWARF_Buffer {
        public:
            poDWARF_Buffer() :
                _pos(0)
            {
            }

            inline void seek(const int pos) { _pos = pos; }
            inline bool eof() const { return _pos < int(_data.size()); }
            inline int pos() const { return _pos; }

            int8_t      readS8();

            uint64_t    readU64();
            uint32_t    readU32();
            uint16_t    readU16();
            uint8_t     readU8();
            int32_t     readLEB128();
            uint32_t    readULEB128();
            void        readString(std::string& str);
            void        readData(std::vector<uint8_t>& arr, const int count);

            void writeData(unsigned char* data, int size);
            void write(uint32_t value);
            void write(uint16_t value);
            void write(uint8_t value);

            void write(int8_t value);
        private:
            int _pos;
            std::vector<unsigned char> _data;

    };

    enum class poDW_Tag {
        ArrayType = 0x1,
        ClassType = 0x2,
        EntryPoint = 0x3,
        EnumType = 0x4,
        FormalParameter = 0x5,
        ImportedDecl = 0x8,
        CompilationUnit = 0x11,
        StructureType = 0x13,
        TypeDef = 0x16,
        BaseType = 0x24,
        ConstType = 0x26
    };

    enum class poDW_AT {
        Sibling = 0x1,
        StatementList = 0x10,
        Language = 0x13,
        CompDir = 0x1b,
        Producer = 0x25,
        Type = 0x49,
        Ranges = 0x55
    };

    enum class poDW_Form {
        Address = 0x1,
        Block2 = 0x3,
        Block4 = 0x4,
        Data2 = 0x5,
        Data4 = 0x6,
        Data8 = 0x7,
        String = 0x8,
        Block = 0x9,
        Block1 = 0xa,
        Data1 = 0xb,
        Flag = 0xc,
        Sdata = 0xd,
        Strp = 0xe,
        UData = 0xf,
        Ref4 = 0x13,
        Sec_offset = 0x17,
        FlagPresent = 0x19,
        Line_strp = 0x1f
    };

    struct poDWARF_Attribute {
        uint32_t _attribute;
        uint32_t _form;
    };

    class poDWARF_Abbr {
        public:
            poDWARF_Abbr() :
                _id(0),
                _tag(0)
            {
            }

            poDWARF_Abbr(const uint32_t id, const uint32_t tag) :
                _id(id),
                _tag(tag)
            {
            }

            uint32_t id() const { return _id; }
            uint32_t tag() const { return _tag; }
            std::vector<poDWARF_Attribute>& attributes() { return _attributes; }
            const std::vector<poDWARF_Attribute>& attributes() const { return _attributes; }

        private:
            uint32_t _id;
            uint32_t _tag;
            std::vector<poDWARF_Attribute> _attributes;
    };

    class poDWARF {
        public:
            
            void write();
            void read_debug_info(unsigned char* data, const int size);
            void read_debug_line(unsigned char* data, const int size);
            void read_debug_abbr(unsigned char* data, const int size);
            void read_debug_str(unsigned char* data, const int size);
            void read_debug_line_str(unsigned char* data, const int size);
            void dump();
        private:
            void writeDebugInfo();
            void writeLineInfo();

            void dumpLineInformation();
            void dumpLineSources(
                const int source_count, 
                const std::vector<uint32_t>& content_codes,
                const std::vector<uint32_t>& form_codes);
         
            poDWARF_Buffer _debug_info;
            poDWARF_Buffer _debug_line;
            poDWARF_Buffer _debug_abbr;
            poDWARF_Buffer _debug_str;
            poDWARF_Buffer _debug_line_str;

            std::vector<poDWARF_Abbr> _abbrs;
            std::unordered_map<int, int> _abbrsMap; /* Abbr ID => Index to _abbrs */
    };
}
