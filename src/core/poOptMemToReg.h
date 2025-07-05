#pragma once
#include "poUses.h"

#include <unordered_map>

//
// Optimization pass which seeks to eliminate uses
// of the IR_ALLOCA which allocates onto the stack
// with operation which use registers.
// This is performed in SSA form.
// 


namespace po
{
    class poModule;
    class poFlowGraph;
    class poBasicBlock;
    class poInstruction;
    class poDom;

    class poOptMemToReg_Alloca
    {
    public:
        poOptMemToReg_Alloca(const poInstructionRef&  source);
        void addUse(const poInstructionRef& ref);
        inline const bool promote() const { return _promote; }
        inline std::vector<poInstructionRef>& uses() { return _uses; }
        inline poInstructionRef& source() { return _source; }

    private:
        std::vector<poInstructionRef> _uses;
        poInstructionRef _source;
        bool _promote;
    };

    class poOptMemToReg
    {
    public:
        void optimize(poModule& module);
        void optimize(poModule& module, poFlowGraph& cfg);

    private:
        void rewritePtr(poInstruction& ins);

        poUses _uses;
        std::vector<poOptMemToReg_Alloca> _alloca;
    };
}
