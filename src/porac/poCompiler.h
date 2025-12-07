#pragma once
#include <vector>
#include <string>
#include "poAsm.h"
#include "poFile.h"

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
        void reportError(const std::string& errorPhase, const std::string& errorText, const int fileId, const int colNum, const int lineNum);

        std::vector<poFile> _files;
        std::vector<std::string> _errors;

        poAsm _assembler;
        bool _debugDump;
    };
}
