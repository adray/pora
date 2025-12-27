#pragma once
#include <cstdint>
#include <string>
#include <vector>

struct ElfProgramHeader64;

namespace po
{
    enum class poELF_SegmentType
    {
        PT_NULL = 0x0,
        PT_LOAD = 0x1,
        PT_DYNAMIC = 0x2,
        PT_INTEROP = 0x3,
        PT_NOTE = 0x4,
        PT_SHLIB = 0x5,
        PT_PHDR = 0x6,
        PT_TLS = 0x7,
        PT_LOOS = 0x60000000,
        PT_HIOS = 0x6FFFFFFF,
        PT_LOPROC = 0x70000000,
        PT_HIPROC = 0x7FFFFFFF
    };

    enum class poELF_SegmentFlags
    {
        PF_X = 0x1,
        PF_W = 0x2,
        PF_R = 0x4
    };

    enum class poELF_SectionType
    {
        SHT_NULL = 0x0,
        SHT_PROGBITS = 0x1,
        SHT_SYMTAB = 0x2,
        SHT_STRTAB = 0x3,
        SHT_RELA = 0x4,
        SHT_HASH = 0x5,
        SHT_DYNAMIC = 0x6,
        SHT_NOTE = 0x7,
        SHT_NOBITS = 0x8,
        SHT_REL = 0x9,
        SHT_SHLIB = 0xA,
        SHT_SYNSYM = 0xB,
        SHT_INIT_ARRAY = 0xE,
        SHT_FINI_ARRAY = 0xF,
        SHT_PREINIT_ARRAY = 0x10,
        SHT_GROUP = 0x11,
        SHT_SYMTAB_SHNDX = 0x12,
        SHT_NUM = 0x13,
        SHT_LOOS = 0x60000000
    };

    class poELF_Segment
    {
        public:
            poELF_Segment(const poELF_SegmentType type, const poELF_SegmentFlags flags);

            inline poELF_SegmentType type() const { return _type; }
            inline poELF_SegmentFlags flags() const { return _flags; }
            inline std::vector<unsigned char>& data() { return _data; }
        private:
            poELF_SegmentType _type;
            poELF_SegmentFlags _flags;
            std::vector<unsigned char> _data;
    };

    class poELF_Section
    {
        public:
            poELF_Section(const poELF_SectionType& type, const std::string& name, const int size);

            inline const poELF_SectionType type() const { return _type; }
            inline const std::string& name() const { return _name; }
            inline const int size() const { return _size; } 
            inline std::vector<unsigned char>& data() { return _data; }
        private:
            poELF_SectionType _type;
            std::vector<unsigned char> _data;
            std::string _name;
            int _size;
    };

    class poELF_Symbol
    {
    public:
        poELF_Symbol(const int id, const int size, const int strPos);
        inline int strPos() const { return _strPos; }
        inline void setName(const std::string& name) { _name = name; }
        inline const std::string& getName() const { return _name; }
        inline const int id() const { return _id; }
    private:
        int _strPos;
        std::string _name;
        int _id;
        int _size;
    };

    class poELF
    {
    public:
        poELF();
        void write(const std::string& filename);
        bool open(const std::string& filename);
        void dump();

        void add(const poELF_SectionType section, const std::string& name, const int size);

        inline const std::vector<poELF_Symbol>& symbols() const { return _symbols; }
        inline poELF_Section& textSection() { return _sections[_textSection]; }
        inline poELF_Section& readOnlySection() { return _sections[_readOnlyDataSection]; }
    private:
        void addProgramHeader(std::vector<ElfProgramHeader64>& headers);

        std::vector<poELF_Symbol> _symbols;
        std::vector<poELF_Segment> _segments;
        std::vector<poELF_Section> _sections;

        int _textSection;
        int _readOnlyDataSection;
    };
}
