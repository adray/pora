#pragma once
#include <unordered_map>
#include "poCFG.h"

// A file for performing constant propagation on SSA form. 

namespace po
{
    class poModule;
    class poFunction;
    class poInstruction;
    class poConstantPool;
    class poDom;

    class poOptProp
    {
    public:
        void optimize(poModule& module);

    private:
        void optimize(poModule& module, poFunction& function);
        void propagate(poModule& module, poInstruction& ins);
        void eliminateBranch(poModule& module, poInstruction& ins, int nodeId, poDom& dom);
        void fixUpPhis(poBasicBlock* bb, poBasicBlock* successor);
        void foldI64(poConstantPool& pool, poInstruction& ins, poInstruction& left, poInstruction& right);
        void foldI32(poConstantPool& pool, poInstruction& ins, poInstruction& left, poInstruction& right);

        std::unordered_map<int, poInstructionRef> _constants;
        std::vector<poBasicBlock*> _blocksToRemove;
    };
}
