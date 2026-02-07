#pragma once
#include "poCallGraph.h"

#include <unordered_map>
#include <string>

namespace po
{
    class poModule;
    class poFlowGraph;
    class poInstruction;
    class poBasicBlock;
    class poFunction;

    class poOptInline
    {
    public:
        void optimize(poModule& module);
        void optimize(poModule& module, poFlowGraph& cfg);
    private:
        bool canInline(poModule& module, const poInstruction& ins);
        bool shouldInline(poModule& module, poFunction& function);
        void inlineFunctionCall(poInstruction& ins, poBasicBlock* bb, poModule& module, poFlowGraph& cfg);
        poBasicBlock* splitBasicBlock(poBasicBlock* bb, const int instructionIndex, poFlowGraph& cfg);

        poCallGraph _graph;
    };
}
