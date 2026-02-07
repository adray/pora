#pragma once
#include <vector>
#include <string>
#include "poAsm.h"
#include "poFile.h"

namespace po
{
    constexpr int OPTIMIZATION_LEVEL_0 = 0;
    constexpr int OPTIMIZATION_LEVEL_1 = 1;
    constexpr int OPTIMIZATION_LEVEL_2 = 2;

    class poCompiler
    {
    public:
        poCompiler()
            :
            _debugDump(false),
            _optimizationLevel(OPTIMIZATION_LEVEL_2)
        {
        }
        void addFile(const std::string& file);
        inline void setDebugDump(const bool debugDump) { _debugDump = debugDump; }
        inline void setOptimizationLevel(const int optimizationLevel) { _optimizationLevel = optimizationLevel; }
        int compile();
        inline const std::vector<std::string>& errors() const { return _errors; }
        inline poAsm& assembler() { return _assembler; }

    private:
        void reportError(const std::string& errorPhase, const std::string& errorText, const int fileId, const int colNum, const int lineNum);

        std::vector<poFile> _files;
        std::vector<std::string> _errors;

        poAsm _assembler;
        bool _debugDump;
        int _optimizationLevel;
    };
}
