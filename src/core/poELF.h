#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace po
{
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

        inline const std::vector<poELF_Symbol>& symbols() const { return _symbols; }
    private:
        std::vector<poELF_Symbol> _symbols; 
    };
}
