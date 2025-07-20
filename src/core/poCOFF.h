#pragma once
#include <string>
#include <vector>

namespace po
{
    class poArchiveLinkMember1
    {
    public:
        void addSymbolOffset(int offset);
        void addSymbolName(const std::string& name);

    private:
        std::vector<int> _offsets;
        std::vector<std::string> _names;
    };

    class poArchiveLinkMember2
    {
    public:
        void addSymbolOffset(int offset);
        void addSymbolIndex(int index);
        void addSymbolName(const std::string& name);

        inline size_t numSymbols() const { return _names.size(); }
        inline const std::string& name(int index) const { return _names[index]; }
        inline const int indicies(int index) const { return _indices[index]; }
        inline const int offset(int index) { return _offsets[index]; }

    private:
        std::vector<int> _offsets;
        std::vector<int> _indices;
        std::vector<std::string> _names;
    };

    class poImport
    {
    public:
        poImport(const int importType, const int importNameType, const uint16_t importOrdinal, const std::string& importName, const std::string& dllName, const uint16_t machine)
            :
            _importType(importType),
            _importNameType(importNameType),
            _importOrdinal(importOrdinal),
            _importName(importName),
            _dllName(dllName),
            _machine(machine)
        {
        }

        inline int importType() const { return _importType; }
        inline int importNameType() const { return _importNameType; }
        inline uint16_t importOrdinal() const { return _importOrdinal; }
        inline const std::string& importName() const { return _importName; }
        inline const std::string& dllName() const { return _dllName; }
        inline uint16_t machine() const { return _machine; }

    private:
        int _importType;
        int _importNameType;
        uint16_t _importOrdinal;
        uint16_t _machine;
        std::string _importName;
        std::string _dllName;
    };

    class poCommonObjectFileFormat
    {
    public:
        bool open(const std::string& filename);
        void dump() const;

        inline const std::vector<poImport>& imports() const { return _imports; }
    private:
        void parseSecondLinkerHeader(std::ifstream& ss);
        void parseFirstLinkerHeader(std::ifstream& ss);

        poArchiveLinkMember1 _linkerMember1;
        poArchiveLinkMember2 _linkerMember2;
        std::vector<poImport> _imports;
    };
}
