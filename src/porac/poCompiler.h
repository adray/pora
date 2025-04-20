#pragma once
#include <vector>
#include <string>
#include "poAsm.h"

namespace po
{
    class poCompiler
    {
    public:
        void addFile(const std::string& file);
        int compile();
        inline const std::vector<std::string>& errors() const { return _errors; }
        inline const poAsm& assembler() const { return _assembler; }

    private:
        std::vector<std::string> _files;
        std::vector<std::string> _errors;

        poAsm _assembler;
    };
}
