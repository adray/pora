#pragma once
#include <vector>
#include <string>
#include "poAsm.h"

namespace po
{
    class poCompiler
    {
    public:
        poCompiler()
            :
            _debugDump(false)
        {
        }
        void addFile(const std::string& file);
        void setDebugDump(const bool debugDump) { _debugDump = debugDump; }
        int compile();
        inline const std::vector<std::string>& errors() const { return _errors; }
        inline poAsm& assembler() { return _assembler; }

    private:
        std::vector<std::string> _files;
        std::vector<std::string> _errors;

        poAsm _assembler;
        bool _debugDump;
    };
}
