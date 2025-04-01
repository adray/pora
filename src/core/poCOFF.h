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

    class poCommonObjectFileFormat
    {
    public:
        bool open(const std::string& filename);

    private:
        void parseSecondLinkerHeader(std::ifstream& ss);
        void parseFirstLinkerHeader(std::ifstream& ss);

        poArchiveLinkMember1 _linkerMember1;
        poArchiveLinkMember2 _linkerMember2;
    };
}
