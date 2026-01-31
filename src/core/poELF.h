#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>

struct ElfProgramHeader64;

namespace po
{
    enum class poELF_SegmentType
    {
        PT_NULL = 0x0,
        PT_LOAD = 0x1,
        PT_DYNAMIC = 0x2,
        PT_INTERP = 0x3,
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
        PF_X = 0x1, // Execute
        PF_W = 0x2, // Write
        PF_R = 0x4  // Read
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

    enum class poELF_SectionFlags
    {
        SHF_WRITE = 0x1,
        SHF_ALLOC = 0x2,
        SHF_EXECINSTR = 0x4,
        SHF_INFO = 0x40,
        SHF_X86_64_LARGE = 0x10000000,
        SHF_TLS = 0x400
    };

    enum class poELF_DynamicTag
    {
        DT_NULL = 0,
        DT_NEEDED = 1,
        DT_PLTRELSIZE = 2,
        DT_PLTGOT = 3,
        DT_HASH = 4,
        DT_STRTAB = 5,
        DT_SYMTAB = 6,
        DT_RELA = 7,
        DT_RELASZ = 8,
        DT_RELAENT = 9,
        DT_STRSZ = 10,
        DT_SYMENT = 11,
        DT_INIT = 12,
        DT_FINI = 13,
        DT_SONAME = 14,
        DT_RPATH = 15,
        DT_SYMBOLIC = 16,
        DT_REL = 17,
        DT_RELSZ = 18,
        DT_RELENT = 19,
        DT_PLTREL = 20,
        DT_DEBUG = 21,
        DT_TEXTREL = 22,
        DT_JMPREL = 23,
        DT_FLAGS = 30
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
            poELF_Section(const poELF_SectionType& type, const poELF_SectionFlags& flags, const std::string& name, const int size, const int entsize, const bool needsSegment);

            inline const poELF_SectionFlags flags() const { return _flags; }
            inline const poELF_SectionType type() const { return _type; }
            inline const std::string& name() const { return _name; }
            inline const int size() const { return _size; } 
            inline std::vector<unsigned char>& data() { return _data; }
            inline int namePos() const { return _namePos; }
            inline void setNamePos(const int pos) { _namePos = pos; }
            inline int virtualAddress() const { return _virtualAddress; }
            inline void setVirtualAddress(const int address) { _virtualAddress = address; }
            inline bool needsSegment() const { return _needsSegment; }
            inline void setNeedsSegment(bool needsSegment) { _needsSegment = needsSegment; }
            inline int entsize() const { return _entsize; }
            inline int link() const { return _link; }
            inline void setLink(const int link) { _link = link; }
            inline int info() const { return _info; }
            inline void setInfo(const int info) { _info = info; }

        private:
            poELF_SectionType _type;
            poELF_SectionFlags _flags;
            std::vector<unsigned char> _data;
            std::string _name;
            int _namePos;
            int _virtualAddress;
            int _size;
            int _entsize;
            int _link;
            int _info;
            bool _needsSegment;
    };

    class poELF_Symbol
    {
    public:
        poELF_Symbol(const int id, const int size, const int strPos);
        inline int strPos() const { return _strPos; }
        inline void setName(const std::string& name) { _name = name; }
        inline const std::string& getName() const { return _name; }
        inline const int id() const { return _id; }
        inline const int type() const { return _type; }
        inline const int addr() const { return _addr; }
        inline void setType(const int type) { _type = type; }
        inline void setAddr(const int addr) { _addr = addr; }
    private:
        int _strPos;
        std::string _name;
        int _id;
        int _size;
        int _type;
        int _addr;
    };

    class poELF_StringTable
    {
        public:
            poELF_StringTable();

            void write(const std::string& str);
            inline const std::vector<unsigned char>& data() { return _data; }

        private:
           std::vector<unsigned char> _data;
    };

    class poELF_HashTable
    {
        public:
            poELF_HashTable();

            void addEntry(const std::string& symbolName);
            void compute();
            void dump();
            inline const std::vector<unsigned char>& data() const { return _data; }
            inline const std::vector<int>& symbols() const { return _symbols; }
            const int lookup(const std::string& symbol);
        private:
            struct Entry {
                std::string name;
                int hash;
                int value;
            };

            int _numBuckets;
            std::vector<Entry> _entries;
            std::vector<int> _buckets;
            std::vector<int> _chains;
            std::vector<int> _symbols;
            std::vector<unsigned char> _data;
            std::unordered_map<std::string, int> _hash;
    };

    class poELF_RelocationTable
    {
        public:
            void add(const int relocationPos, const std::string& symbolName);
            void build(const int offset, poELF_HashTable& hash);

            inline const std::vector<unsigned char>& data() const { return _data; }
        private:
            struct Entry {
                std::string name;
                int relocationPos;
                int symbolId;
            };

            std::vector<Entry> _entries;
            std::vector<unsigned char> _data;
    };

    class poELF
    {
    public:
        poELF();
        void write(const std::string& filename);
        bool open(const std::string& filename);
        void dump();
        void initializeSections();

        void add(const poELF_SectionType section, const std::string& name, const int size);
        void addSymbol(const std::string& name, const int id);
        void addLibrary(const std::string& name);
        void addPltRelocation(const int relocationPos, const std::string& symbol);

        inline void setEntryPoint(const int pos) { _entryPoint = pos; }
        inline const std::vector<poELF_Symbol>& symbols() const { return _symbols; }
        inline poELF_Section& textSection() { return _sections[_textSection]; }
        inline poELF_Section& readOnlySection() { return _sections[_readOnlyDataSection]; }
        inline poELF_Section& initializedDataSection() { return _sections[_initializedDataSection]; }
        inline poELF_Section& pltDataSection() { return _sections[_plt]; }
        inline poELF_Section& pltGotDataSection() { return _sections[_got_plt]; }
    private:
        void addPHDRSegment(std::vector<ElfProgramHeader64>& headers);
        void addProgramHeader(poELF_Section& section, std::vector<ElfProgramHeader64>& headers);
        void addInterpSegment(poELF_Section& section, std::vector<ElfProgramHeader64>& headers);
        void addDynamicSegment(poELF_Section& section, std::vector<ElfProgramHeader64>& headers);
        void buildDynamicSection();
        void buildStringTable();
        void buildSymbolTable();
        void buildHashTable();
        void buildRelocationTable();

        std::vector<poELF_Symbol> _symbols;
        std::vector<poELF_Segment> _segments;
        std::vector<poELF_Section> _sections;
        std::vector<int> _libraries;
        poELF_StringTable _stringTable;
        poELF_HashTable _hashTable;
        poELF_RelocationTable _pltTable;

        int _entryPoint;
        int _strtable;
        int _shstrtable;
        int _hashtable;
        int _textSection;
        int _readOnlyDataSection;
        int _initializedDataSection;
        int _dynamicSection;
        int _symtable;
        int _reltable;
        int _relatable;
        int _interp;
        int _plt;
        int _plt_rela;
        int _got_plt;
    };
}
